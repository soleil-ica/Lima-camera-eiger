//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2015
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>

#include "eigerapi/CurlLoop.h"
#include "eigerapi/EigerDefines.h"
//Lock class
class Lock
{
public:
  Lock(pthread_mutex_t *aLock,bool aLockFlag = true) :
    _lock(aLock),_lockFlag(false)
  {if(aLockFlag) lock();}

    ~Lock() {unLock();}
    inline void lock() 
    {
      if(!_lockFlag)
	while(pthread_mutex_lock(_lock)) ;
      _lockFlag = true;
    }
    inline void unLock()
    {
      if(_lockFlag)
	{
	  _lockFlag = false;
	  pthread_mutex_unlock(_lock);
	}
    }
  private:
    pthread_mutex_t *_lock;
    volatile bool   _lockFlag;
  };

using namespace eigerapi;

// Constant
double const CurlLoop::FutureRequest::TIMEOUT = 15;

struct CURL_INIT
{
  CURL_INIT()
  {
    curl_global_init(CURL_GLOBAL_ALL);
  }
  ~CURL_INIT()
  {
    curl_global_cleanup();
  }
} global_init;

CurlLoop::CurlLoop() : 
  m_running(false),
  m_quit(false),
  m_thread_id(0),
  m_curl_delay_ms(50)
{
  if(pthread_mutex_init(&m_lock,NULL))
    THROW_EIGER_EXCEPTION("pthread_mutex_init","Can't initialize the lock");
  if(pthread_cond_init(&m_cond,NULL))
    THROW_EIGER_EXCEPTION("pthread_cond_init",
			  "Can't initialize the variable condition");

  if(pipe(m_pipes))
    THROW_EIGER_EXCEPTION("pipe","Can't create pipe");
  
  //set pipe none blocking
  fcntl(m_pipes[0],F_SETFL,O_NONBLOCK);
  fcntl(m_pipes[1],F_SETFL,O_NONBLOCK);

  pthread_create(&m_thread_id,NULL,_runFunc,this);
}

CurlLoop::~CurlLoop()
{
  quit();

  pthread_mutex_destroy(&m_lock);
  pthread_cond_destroy(&m_cond);
}

void CurlLoop::quit()
{
  if(!m_thread_id) return;

  Lock alock(&m_lock);
  m_quit = true;
  write(m_pipes[1],"|",1);
  pthread_cond_broadcast(&m_cond);
  alock.unLock();

  if(m_thread_id > 0)
    pthread_join(m_thread_id,NULL);

  close(m_pipes[1]),close(m_pipes[0]);
  m_thread_id = 0;
}

void CurlLoop::add_request(std::shared_ptr<CurlLoop::FutureRequest> new_request)
{
  Lock alock(&m_lock);
  if(write(m_pipes[1],"|",1) == -1 && errno != EAGAIN)
    THROW_EIGER_EXCEPTION("write into pipe","synchronization failed");

  m_new_requests.push_back(new_request);
  new_request->m_status = FutureRequest::RUNNING;

  pthread_cond_broadcast(&m_cond);
}

void CurlLoop::cancel_request(std::shared_ptr<CurlLoop::FutureRequest> request)
{
  Lock alock(&m_lock);
  
  m_cancel_requests.push_back(request);
  alock.unLock();

  Lock req_lock(&request->m_lock);
  request->m_status = FutureRequest::CANCEL;
}

void CurlLoop::set_curl_delay_ms(double curl_delay_ms)
{
    m_curl_delay_ms = curl_delay_ms;
}

void* CurlLoop::_runFunc(void *curlloopPt)
{
  ((CurlLoop*)curlloopPt)->_run();
  return NULL;
}

void CurlLoop::_run()
{
  CURLM* multi_handle = curl_multi_init();
  Lock lock(&m_lock);
  while(!m_quit)
    {
      lock.lock();
      while(!m_quit && 
	    m_pending_requests.empty() && m_new_requests.empty())
	{
	  m_running = false;
	  pthread_cond_broadcast(&m_cond);
	  pthread_cond_wait(&m_cond,&m_lock);
	}
      if(m_quit) break;
      m_running = true;
      //Add all new requests
      for(ListRequests::iterator i = m_new_requests.begin();
	  i != m_new_requests.end();++i)
	{
	  std::pair<MapRequests::iterator,bool> result = 
	    m_pending_requests.insert(MapRequests::value_type((*i)->m_handle,*i));
	  if(result.second)
	    curl_multi_add_handle(multi_handle,(*i)->m_handle);
	}
      m_new_requests.clear();
      lock.unLock();

      fd_set fdread;
      fd_set fdwrite;
      fd_set fdexcep;

      FD_ZERO(&fdread);
      FD_SET(m_pipes[0],&fdread);

      FD_ZERO(&fdwrite);
      FD_ZERO(&fdexcep);

      //get curl timeout
      struct timeval timeout{1,0};
      struct timeval* timeoutPt;
      long curl_timeout = -1;
      curl_multi_timeout(multi_handle,&curl_timeout);
      if(curl_timeout >=0)
	{
	  timeout.tv_sec = curl_timeout / 1000;
	  timeout.tv_usec = (curl_timeout % 1000) * 1000;
	  timeoutPt = &timeout;
	}
      else
	timeoutPt = NULL;

      int max_fd = -1;
      CURLMcode mc = curl_multi_fdset(multi_handle,
				      &fdread,&fdwrite,&fdexcep,
				      &max_fd);
      if(mc != CURLM_OK)
	{
	  std::cerr << "Big problem occurred in curl loop " <<
	    __FILE__ << ":" << __LINE__ << ", exit" << std::endl;
	  break;
	}
      // fds are not ready in curl, wait 100ms
      int nb_event = 0;
      if(max_fd == -1)
      {
	    usleep(m_curl_delay_ms * 1000);
	    //std::cout << "==================== sleeped m_curl_delay_ms ms waiting for curl =====================" << std::endl;
	  }
      else
      {
        //std::cout << "==================== Curl is finally ready ... =====================" << std::endl;
	    nb_event = select(max_fd + 1,&fdread,&fdwrite,&fdexcep,timeoutPt);
	  }
	
      if(nb_event == -1)
	{
	  if(errno != EINTR)
	    {
	      std::cerr << "Big problem occurred in curl loop " << 
		__FILE__ << ":" << __LINE__ << ", exit" << std::endl;
	      break;
	    }
	}
      else if(nb_event >= 0 || max_fd == -1)
	{
	  // flush pipe
	  char buffer[1024];
	  if(read(m_pipes[0],buffer,sizeof(buffer)) == -1 && errno != EAGAIN)
	    std::cerr << "Warning: something strange happen! (" << 
	      errno << ',' << strerror(errno) << ')' <<
	      __FILE__ << ":" << __LINE__ << ", exit" << std::endl;

	  int nb_running;
	  curl_multi_perform(multi_handle,&nb_running);

	  lock.lock();
	  CURLMsg *msg;
	  int msg_left;
	  while((msg = curl_multi_info_read(multi_handle,&msg_left)))
	    {
	      if(msg->msg == CURLMSG_DONE)
		{
		  curl_multi_remove_handle(multi_handle,msg->easy_handle);
		  MapRequests::iterator request = m_pending_requests.find(msg->easy_handle);
		  if(request == m_pending_requests.end())
		    {
		      std::cerr << "Warning CurlLoop: something strange happen! " <<
			__FILE__ << ":" << __LINE__ << ", exit" << std::endl;
		    }
		  else
		    {
		      std::shared_ptr<FutureRequest> req = request->second;
		      m_pending_requests.erase(request);
		      lock.unLock();

		      Lock request_lock(&req->m_lock);
		      if(req->m_status != FutureRequest::CANCEL)
			{
			  switch(msg->data.result)
			    {
			    case CURLE_OK:
			      req->m_status = FutureRequest::OK;
			      break;
			    default: // error
			      req->m_status = FutureRequest::ERROR;
			      req->m_error_code = 
				curl_easy_strerror(msg->data.result);
			      break;
			    }
			}
		      pthread_cond_broadcast(&req->m_cond);
		      if(req->m_cbk)
			(*req->m_cbk)->status_changed(req->m_status);
		      req->_request_finished();

		      lock.lock();
		    }
		}
	    }
	  //Remove canceled request
	  for(ListRequests::iterator i = m_cancel_requests.begin();
	      i != m_cancel_requests.end();++i)
	    {
	      MapRequests::iterator request = m_pending_requests.find((*i)->m_handle);
	      if(request != m_pending_requests.end())
		{
		  curl_multi_remove_handle(multi_handle,(*i)->m_handle);
		  m_pending_requests.erase(request);
		}
	    }
	  m_cancel_requests.clear();
	}
    }
  //cleanup
  for(MapRequests::iterator i = m_pending_requests.begin();
      i != m_pending_requests.end();++i)
    curl_multi_remove_handle(multi_handle,i->first);
  m_pending_requests.clear();
  curl_multi_cleanup(multi_handle);
}


CurlLoop::FutureRequest::FutureRequest(const std::string& url) :
  m_status(IDLE),
  m_cbk(NULL),
  m_url(url)
{
  if(pthread_mutex_init(&m_lock,NULL))
    THROW_EIGER_EXCEPTION("pthread_mutex_init","Can't initialize the lock");
  if(pthread_cond_init(&m_cond,NULL))
    THROW_EIGER_EXCEPTION("pthread_cond_init",
			  "Can't initialize the variable condition");
  m_handle = curl_easy_init();
  curl_easy_setopt(m_handle,CURLOPT_URL, url.c_str());
  curl_easy_setopt(m_handle, CURLOPT_PROXY, "");
#ifdef DEBUG
  curl_easy_setopt(m_handle, CURLOPT_VERBOSE, 1L);
#endif
}

CurlLoop::FutureRequest::~FutureRequest()
{
  curl_easy_cleanup(m_handle);
  delete m_cbk;
}

void CurlLoop::FutureRequest::wait(double timeout,bool lock_flag) const
{
  Lock lock(&m_lock,lock_flag);

  if(m_status == IDLE)		// weird init
    THROW_EIGER_EXCEPTION("weird initialization","status == IDLE");

  int retcode = 0;
  if(timeout >= 0.)
    {
      struct timeval now;
      struct timespec waitTimeout;
      gettimeofday(&now,NULL);
      waitTimeout.tv_sec = now.tv_sec + long(timeout);
      waitTimeout.tv_nsec = (now.tv_usec * 1000) + 
	long((timeout - long(timeout)) * 1e9);
      if(waitTimeout.tv_nsec >= 1000000000L) // Carry
	++waitTimeout.tv_sec,waitTimeout.tv_nsec -= 1000000000L;

      while(retcode != ETIMEDOUT && m_status == RUNNING)
	retcode = pthread_cond_timedwait(&m_cond,&m_lock,&waitTimeout);

      if(retcode == ETIMEDOUT)
	THROW_EIGER_EXCEPTION("wait_status","timeout");
    }
  else
    while(m_status == RUNNING)
      pthread_cond_wait(&m_cond,&m_lock);

  if(m_status == ERROR)
    THROW_EIGER_EXCEPTION("wait_status",m_error_code.c_str());
}

CurlLoop::FutureRequest::Status
CurlLoop::FutureRequest::get_status() const
{
  Lock lock(&m_lock);
  return m_status;
}

void CurlLoop::FutureRequest::register_callback(std::shared_ptr<Callback>& cbk)
{
  Lock lock(&m_lock);
  m_cbk = new std::shared_ptr<Callback>(cbk);
  Status status = m_status;
  lock.unLock();

  if(status != RUNNING)
    cbk->status_changed(status);
}
