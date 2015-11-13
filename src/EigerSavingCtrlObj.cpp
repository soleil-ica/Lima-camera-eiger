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
#include <algorithm>
#include "EigerSavingCtrlObj.h"

#include <eigerapi/Requests.h>
#include <eigerapi/EigerDefines.h>

using namespace lima;
using namespace lima::Eiger;
using namespace eigerapi;

const int MAX_SIMULTANEOUS_DOWNLOAD = 4;
/*----------------------------------------------------------------------------
			     HDF5 HEADER
----------------------------------------------------------------------------*/
struct HeaderKey2Index
{
  const char*		key_name;
  Requests::PARAM_NAME	param_name;
};

static HeaderKey2Index available_header[] = {
  {"beam_center_x",Requests::HEADER_BEAM_CENTER_X},
  {"beam_center_y",Requests::HEADER_BEAM_CENTER_Y},
  {"chi",Requests::HEADER_CHI},
  {"chi_end",Requests::HEADER_CHI_END},
  {"chi_range_average",Requests::HEADER_CHI_RANGE_AVERAGE},
  {"chi_range_total",Requests::HEADER_CHI_RANGE_TOTAL},
  {"detector_distance",Requests::HEADER_DETECTOR_DISTANCE},
  {"kappa",Requests::HEADER_KAPPA},
  {"kappa_end",Requests::HEADER_KAPPA_END},
  {"kappa_range_average",Requests::HEADER_KAPPA_RANGE_AVERAGE},
  {"kappa_range_total",Requests::HEADER_KAPPA_RANGE_TOTAL},
  {"omega",Requests::HEADER_OMEGA},
  {"omega_end",Requests::HEADER_OMEGA_END},
  {"omega_range_average",Requests::HEADER_OMEGA_RANGE_AVERAGE},
  {"omega_range_total",Requests::HEADER_OMEGA_RANGE_TOTAL},
  {"phi",Requests::HEADER_PHI},
  {"phi_end",Requests::HEADER_PHI_END},
  {"phi_range_average",Requests::HEADER_PHI_RANGE_AVERAGE},
  {"phi_range_total",Requests::HEADER_PHI_RANGE_TOTAL},
  {"wavelength",Requests::HEADER_WAVELENGTH},
};
/*----------------------------------------------------------------------------
			    Polling thread
----------------------------------------------------------------------------*/
class SavingCtrlObj::_PollingThread : public Thread
{
  DEB_CLASS_NAMESPC(DebModCamera,"SavingCtrlObj","_PollingThread");
public:
  _PollingThread(SavingCtrlObj&,eigerapi::Requests*);
  virtual ~_PollingThread();
protected:
  virtual void threadFunction();
private:
  SavingCtrlObj&	m_saving;
  eigerapi::Requests*	m_requests;
};

SavingCtrlObj::SavingCtrlObj(Camera& cam) :
  HwSavingCtrlObj(HwSavingCtrlObj::COMMON_HEADER,false),
  m_cam(cam),
  m_nb_file_to_watch(0),
  m_nb_file_transfer_started(0),
  m_concurrent_download(0),
  m_poll_master_file(false),
  m_quit(false)
{
  m_polling_thread = new _PollingThread(*this,this->m_cam.m_requests);
  m_polling_thread->start();
  // Known keys for common header
  int nb_header_key = sizeof(available_header) / sizeof(HeaderKey2Index);
  for(int i = 0;i < nb_header_key;++i)
    {
      HeaderKey2Index& index = available_header[i];
      m_availables_header_keys[index.key_name] = index.param_name;
    }
}

/*----------------------------------------------------------------------------
			End download callback
----------------------------------------------------------------------------*/
class SavingCtrlObj::_EndDownloadCallback : public CurlLoop::FutureRequest::Callback
{
  DEB_CLASS_NAMESPC(DebModCamera,"SavingCtrlObj","_EndDownloadCallback");
public:
  _EndDownloadCallback(SavingCtrlObj&,const std::string &filename);

  virtual void status_changed(CurlLoop::FutureRequest::Status);
private:
  SavingCtrlObj&	m_saving;
  std::string		m_filename;
};

/*----------------------------------------------------------------------------
			    SavingCtrlObj
----------------------------------------------------------------------------*/
SavingCtrlObj::~SavingCtrlObj()
{
  delete m_polling_thread;
}

void SavingCtrlObj::getPossibleSaveFormat(std::list<std::string> &format_list) const
{
  format_list.push_back(HwSavingCtrlObj::HDF5_FORMAT_STR);
}

void SavingCtrlObj::setCommonHeader(const HwSavingCtrlObj::HeaderMap& header)
{
  DEB_MEMBER_FUNCT();

  std::list<std::shared_ptr<Requests::Param>> pending_request;
  for(HwSavingCtrlObj::HeaderMap::const_iterator i = header.begin();
      i != header.end();++i)
    {
      std::map<std::string,int>::iterator header_index = m_availables_header_keys.find(i->first);
      if(header_index == m_availables_header_keys.end())
	THROW_HW_ERROR(Error) << "Header key: " << i->first << " not yet managed ";
      pending_request.push_back(m_cam.m_requests->set_param(Requests::PARAM_NAME(header_index->second),
							    i->second));
    }

  try
    {
      for(std::list<std::shared_ptr<Requests::Param>>::iterator i = pending_request.begin();
	  i != pending_request.end();++i)
	(*i)->wait();
    }
  catch(const eigerapi::EigerException &e)
    {
       for(std::list<std::shared_ptr<Requests::Param>>::iterator i = pending_request.begin();
	  i != pending_request.end();++i)
	 m_cam.m_requests->cancel(*i);
       THROW_HW_ERROR(Error) << e.what();
    }
}

void SavingCtrlObj::resetCommonHeader()
{
  // todo
}

void SavingCtrlObj::setSerieId(int value)
{
  DEB_MEMBER_FUNCT();
  DEB_PARAM() << DEB_VAR1(value);

  AutoMutex lock(m_cond.mutex());
  m_serie_id = value;
}

SavingCtrlObj::Status SavingCtrlObj::getStatus()
{
  DEB_MEMBER_FUNCT();
  AutoMutex lock(m_cond.mutex());
  bool status = m_poll_master_file ||
    (m_nb_file_to_watch != m_nb_file_transfer_started);
  DEB_RETURN() << DEB_VAR2(status,m_error_msg);
  if(m_error_msg.empty())
    return status ? RUNNING : IDLE;
  else
    return ERROR;
}

void SavingCtrlObj::stop()
{
  DEB_MEMBER_FUNCT();
  AutoMutex lock(m_cond.mutex());
  m_nb_file_transfer_started = m_nb_file_to_watch = 0;
  m_poll_master_file = false;
}

void SavingCtrlObj::_setActive(bool active)
{
  DEB_MEMBER_FUNCT();

  const char *active_str = active ? "enabled" : "disabled";
  std::shared_ptr<Requests::Param> active_req = 
    m_cam.m_requests->set_param(Requests::FILEWRITER_MODE,
				active_str);
  DEB_TRACE() << "FILEWRITER_MODE:" << DEB_VAR1(active_str);
  active_req->wait();
}

void SavingCtrlObj::_prepare()
{
  DEB_MEMBER_FUNCT();

  int frames_per_file = int(m_frames_per_file);
  std::shared_ptr<Requests::Param> nb_image_per_file_req = 
    m_cam.m_requests->set_param(Requests::NIMAGES_PER_FILE,
				frames_per_file);
  DEB_TRACE() << "NIMAGES_PER_FILE:" << DEB_VAR1(frames_per_file);

  std::shared_ptr<Requests::Param> name_pattern_req = 
    m_cam.m_requests->set_param(Requests::FILEWRITER_NAME_PATTERN,m_prefix);
  DEB_TRACE() << "FILEWRITER_NAME_PATTERN" << DEB_VAR1(m_prefix);

  nb_image_per_file_req->wait(),name_pattern_req->wait();

  AutoMutex lock(m_cond.mutex());
  m_nb_file_transfer_started = m_nb_file_to_watch = 0;
  m_poll_master_file = true;
}

void SavingCtrlObj::_start()
{
  DEB_MEMBER_FUNCT();
  DEB_PARAM() << DEB_VAR1(m_active);

  int nb_frames;	m_cam.getNbFrames(nb_frames);
  double expo_time;	m_cam.getExpTime(expo_time);

  AutoMutex lock(m_cond.mutex());
  m_nb_file_transfer_started = 0;
  m_nb_file_to_watch = nb_frames / m_frames_per_file;
  if(nb_frames % m_frames_per_file) ++m_nb_file_to_watch;

  m_waiting_time = (expo_time * std::min(nb_frames,int(m_frames_per_file))) / 2.;
  
  m_cond.broadcast();
  
  DEB_TRACE() << DEB_VAR2(m_nb_file_to_watch,m_waiting_time);
}
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

SavingCtrlObj::_PollingThread::_PollingThread(SavingCtrlObj& saving,
					      eigerapi::Requests* requests) :
  m_saving(saving),
  m_requests(requests)
{
  pthread_attr_setscope(&m_thread_attr,PTHREAD_SCOPE_PROCESS);
}

SavingCtrlObj::_PollingThread::~_PollingThread()
{
  AutoMutex lock(m_saving.m_cond.mutex());
  m_saving.m_quit = true;
  m_saving.m_cond.broadcast();
  lock.unlock();

  join();
}

void SavingCtrlObj::_PollingThread::threadFunction()
{
  DEB_MEMBER_FUNCT();
  AutoMutex lock(m_saving.m_cond.mutex());
  
  while(!m_saving.m_quit)
    {
      while(!m_saving.m_quit &&
	    (m_saving.m_concurrent_download >= MAX_SIMULTANEOUS_DOWNLOAD ||
	     (!m_saving.m_poll_master_file &&
	      (m_saving.m_nb_file_to_watch == 
	       m_saving.m_nb_file_transfer_started))))
	{
	  m_saving.m_cond.broadcast();
	  m_saving.m_cond.wait();
	}

      if(m_saving.m_quit) break;
      std::string prefix = m_saving.m_prefix;
      std::string directory = m_saving.m_directory;

      char nb_series[32];
      snprintf(nb_series,sizeof(nb_series),"%d",m_saving.m_serie_id);
      
      int total_nb_frames; m_saving.m_cam.getNbFrames(total_nb_frames);
      
      int frames_per_file = m_saving.m_frames_per_file;
      lock.unlock();

      Requests::Param::Value files;
      //Ls request
      std::shared_ptr<Requests::Param> ls_req = 
	m_requests->get_param(Requests::FILEWRITER_LS);
      try
	{
	  files = ls_req->get();
	}
      catch(eigerapi::EigerException& e)
	{
	  m_requests->cancel(ls_req);
	  DEB_WARNING() << "ls failed, continue: " << e.what();
	  continue;
	}

      // try to download master file
      lock.lock();
      if(m_saving.m_poll_master_file)
	{
	  std::ostringstream src_file_name;
	  src_file_name << prefix <<  "_master.h5";
	  bool master_file_found = false;
	  for(std::vector<std::string>::iterator i = files.string_array.begin();
	      !master_file_found && i != files.string_array.end();++i)
	    master_file_found = *i == src_file_name.str();

	  if(master_file_found)
	    {
	      std::string master_file_name = prefix + "_master.h5";
	      std::string dest_path = directory + "/" + master_file_name;
	      std::shared_ptr<Requests::Transfer> master_file_req;
	      try
		{
		  master_file_req = m_requests->start_transfer(src_file_name.str(),dest_path);
		}
	      catch(eigerapi::EigerException& e)
		{
		  Event *event = new Event(Hardware,Event::Error, Event::Saving,
					   Event::SaveOpenError,e.what());
		  lock.unlock();
		  m_saving.m_cam.reportEvent(event);
		  lock.lock();
		  // stop the loop
		  m_saving.m_nb_file_to_watch = m_saving.m_nb_file_transfer_started = 0;
		  continue;
		}
	      std::shared_ptr<CurlLoop::FutureRequest::Callback>
		end_cbk(new _EndDownloadCallback(m_saving,src_file_name.str()));
	      lock.unlock();
	      master_file_req->register_callback(end_cbk);
	      lock.lock();
	      m_saving.m_poll_master_file = false;
	      ++m_saving.m_concurrent_download;
	    }
	}
      
      if(m_saving.m_nb_file_transfer_started < m_saving.m_nb_file_to_watch)
	{
	  int next_file_nb = m_saving.m_nb_file_transfer_started + 1;

	  std::sort(files.string_array.begin(),files.string_array.end());
	  char file_nb[32];
	  snprintf(file_nb,sizeof(file_nb),"%.6d",next_file_nb);

	  std::ostringstream src_file_name;
	  src_file_name << prefix << "_data_"  << file_nb << ".h5";
	  
	  //init find the first file_name of the list
	  std::vector<std::string>::iterator file_name = files.string_array.begin();
	  for(;file_name != files.string_array.end();++file_name)
	    if(*file_name == src_file_name.str()) break;

	  for(;file_name != files.string_array.end() && 
		m_saving.m_concurrent_download < MAX_SIMULTANEOUS_DOWNLOAD;
	      ++file_name,++next_file_nb)
	    {

	      snprintf(file_nb,sizeof(file_nb),"%.6d",next_file_nb);
	      src_file_name.clear();src_file_name.seekp(0);
	      src_file_name << prefix << "_data_"
			    << file_nb << ".h5";
	      if(*file_name == src_file_name.str()) // will start the transfer
		{
		  if(next_file_nb > m_saving.m_nb_file_to_watch)
		    {
		      DEB_WARNING() << "Something weird happened " 
				    << DEB_VAR2(next_file_nb,m_saving.m_nb_file_to_watch);
		      break;
		    }

		  DEB_TRACE() << "Start transfer file: " << DEB_VAR1(*file_name);
		  std::string dest_path = directory + "/" + src_file_name.str();
		  std::shared_ptr<Requests::Transfer> file_req;
		  try
		    {
		      file_req = m_requests->start_transfer(src_file_name.str(),dest_path);
		    }
		  catch(eigerapi::EigerException& e)
		    {
		      Event *event = new Event(Hardware,Event::Error, Event::Saving,
					       Event::SaveOpenError,e.what());
		      lock.unlock();
		      m_saving.m_cam.reportEvent(event);
		      lock.lock();
		      // stop the loop
		      m_saving.m_nb_file_to_watch = m_saving.m_nb_file_transfer_started = 0;
		      break;
		    }

		  ++m_saving.m_nb_file_transfer_started,++m_saving.m_concurrent_download;
		  std::shared_ptr<CurlLoop::FutureRequest::Callback>
		    end_cbk(new _EndDownloadCallback(m_saving,src_file_name.str()));
		  lock.unlock();
		  file_req->register_callback(end_cbk);
		  lock.lock();

		  if(m_saving.m_callback)
		    {
		      int written_frame = m_saving.m_nb_file_transfer_started * frames_per_file;
		      if(written_frame > total_nb_frames)
			written_frame = total_nb_frames;
		      
		      //lima index start at 0
		      --written_frame;
		      lock.unlock();
		      bool continueFlag = m_saving.m_callback->newFrameWritten(written_frame);
		      lock.lock();
		      if(!continueFlag) // stop the loop
			m_saving.m_nb_file_to_watch = m_saving.m_nb_file_transfer_started = 0;
		    }
		}
	      else
		break;
	    }
	}

      m_saving.m_cond.wait(m_saving.m_waiting_time);
    }
}

/*----------------------------------------------------------------------------
		      class _EndDownloadCallback
----------------------------------------------------------------------------*/
SavingCtrlObj::_EndDownloadCallback::_EndDownloadCallback(SavingCtrlObj& saving,
							  const std::string& filename) :
  m_saving(saving),
  m_filename(filename)
{
}

void SavingCtrlObj::_EndDownloadCallback::
status_changed(CurlLoop::FutureRequest::Status status)
{
  DEB_MEMBER_FUNCT();
  AutoMutex lock(m_saving.m_cond.mutex());
  if(status != CurlLoop::FutureRequest::OK)
    {
      m_saving.m_error_msg = "Failed to download file: ";
      m_saving.m_error_msg += m_filename;
      DEB_ERROR() << m_saving.m_error_msg;
      //Stop the polling
      m_saving.m_poll_master_file = false;
      m_saving.m_nb_file_transfer_started = m_saving.m_nb_file_to_watch = 0;
    }
  
  --m_saving.m_concurrent_download;
  m_saving.m_cond.broadcast();
}
