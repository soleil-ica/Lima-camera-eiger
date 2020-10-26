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
#include <pthread.h>
#include <curl/curl.h>

#include <memory>
#include <map>
#include <list>
#include <string>

namespace eigerapi
{
  class CurlLoop
  {
  public:
    class FutureRequest
    {
      friend class CurlLoop;
    public:
      static double const TIMEOUT;
      enum Status {IDLE,CANCEL,RUNNING,OK,ERROR};
      virtual ~FutureRequest();
      
      void wait(double timeout = TIMEOUT,bool lock = true) const;
      Status get_status() const;

      class Callback
      {
      public:
	Callback() {};
	virtual ~Callback() {}
	virtual void status_changed(FutureRequest::Status) = 0;
      };
      
      void register_callback(std::shared_ptr<Callback>&);

      CURL* get_handle() {return m_handle;}
      FutureRequest(const std::string& url);
    protected:
      virtual void _request_finished() {};

      CURL*				m_handle;
      Status				m_status;
      std::string			m_error_code;
      // Synchro
      mutable pthread_mutex_t		m_lock;
      mutable pthread_cond_t		m_cond;
      std::shared_ptr<Callback>*	m_cbk;
      std::string			m_url;
    };
    
    CurlLoop();
    ~CurlLoop();

    void quit();		// quit the curl loop

    void add_request(std::shared_ptr<FutureRequest>);
    void cancel_request(std::shared_ptr<FutureRequest>);
    void set_curl_delay_ms(double);
  private:
    typedef std::map<CURL*,std::shared_ptr<FutureRequest> > MapRequests;
    typedef std::list<std::shared_ptr<FutureRequest> > ListRequests;
    static void* _runFunc(void*);
    void _run();

    // Synchro
    int			m_pipes[2];
    volatile bool	m_running;
    volatile bool	m_quit;
    pthread_mutex_t	m_lock;
    pthread_cond_t	m_cond;
    pthread_t		m_thread_id;
    //Pending Request
    MapRequests		m_pending_requests;
    ListRequests	m_new_requests;
    ListRequests	m_cancel_requests;
    double          m_curl_delay_ms;
  };
}
