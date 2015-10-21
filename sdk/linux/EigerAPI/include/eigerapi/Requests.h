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
#include <string>
#include <map>
#include <vector>

#include "eigerapi/CurlLoop.h"

namespace eigerapi
{
  class Requests
  {
  public:
    class Command : public CurlLoop::FutureRequest
    {
      friend class Requests;
    public:
      Command(const std::string& url);
      virtual ~Command();

      int get_serie_id();
    private:
      static size_t _write_callback(char *ptr,size_t size,size_t nmemb,Command*);
      void _fill_request();
      char m_data[128];
    };
  
    class Param : public CurlLoop::FutureRequest
    {
      friend class Requests;
    public:
      enum VALUE_TYPE {BOOL,DOUBLE,INT,UNSIGNED,STRING,STRING_ARRAY};
      struct Value
      {
	VALUE_TYPE type;
	union
	{
	  bool bool_val;
	  int  int_val;
	  unsigned int unsigned_val;
	  double double_val;
	}data;
	std::string string_val;
	std::vector<std::string> string_array;
      };
	
      Param(const std::string& url);
      virtual ~Param();
      Value get(double timeout = CurlLoop::FutureRequest::TIMEOUT,
		bool lock = true);
    private:
      void _fill_get_request();
      template <class T>
      void _fill_set_request(const T& value);

      void _set_return_value(bool&);
      void _set_return_value(double&);
      void _set_return_value(int&);
      void _set_return_value(unsigned int&);
      void _set_return_value(std::string&);

      virtual void _request_finished();

      static size_t _write_callback(char*, size_t, size_t, void*);

      char*			m_data_buffer;
      int			m_data_size;
      int			m_data_memorysize;
      struct curl_slist*	m_headers;
      VALUE_TYPE		m_return_type;
      void*			m_return_value;
    };
    
    class Transfer : public CurlLoop::FutureRequest
    {
      friend class Requests;
    public:
      Transfer(Requests& requests,
	       const std::string& url,
	       const std::string& target_path,
	       bool delete_after_transfer = true,
	       int buffer_write_size = 64 * 1024);
      virtual ~Transfer();
    private:
      static size_t _write(void *ptr, size_t size, size_t nmemb,Transfer*);
      virtual void _request_finished();
      
      Requests&	m_requests;
      bool	m_delete_after_transfer;
      long	m_download_size;
      FILE*	m_target_file;
      void*	m_buffer;
    };

    enum COMMAND_NAME {INITIALIZE,ARM, DISARM,TRIGGER,CANCEL,ABORT};
    enum PARAM_NAME {TEMP,
		     HUMIDITY,
		     DETECTOR_STATUS,
		     PIXELDEPTH,
		     X_PIXEL_SIZE,
		     Y_PIXEL_SIZE,
		     DETECTOR_WITDH,
		     DETECTOR_HEIGHT,
		     DESCRIPTION,
		     DETECTOR_NUMBER,
		     DETECTOR_READOUT_TIME,
		     EXPOSURE,
		     FRAME_TIME,
		     TRIGGER_MODE,
		     COUNTRATE_CORRECTION,
		     FLATFIELD_CORRECTION,
		     EFFICIENCY_CORRECTION,
		     PIXEL_MASK,
		     THRESHOLD_ENERGY,
		     VIRTUAL_PIXEL_CORRECTION,
		     PHOTON_ENERGY,
		     NIMAGES,
		     AUTO_SUMMATION,
		     FILEWRITER_MODE,
		     FILEWRITER_COMPRESSION,
		     FILEWRITER_NAME_PATTERN,
		     NIMAGES_PER_FILE,
		     FILEWRITER_STATUS,
		     FILEWRITER_ERROR,
		     FILEWRITER_TIME,
		     FILEWRITER_BUFFER_FREE,
		     FILEWRITER_LS,
		     STREAM_MODE,
		     STREAM_HEADER_DETAIL,
    };

    Requests(const std::string& address);
    ~Requests();

    std::shared_ptr<Command> get_command(COMMAND_NAME);
    std::shared_ptr<Param> get_param(PARAM_NAME);
    std::shared_ptr<Param> get_param(PARAM_NAME,bool&);
    std::shared_ptr<Param> get_param(PARAM_NAME,double&);
    std::shared_ptr<Param> get_param(PARAM_NAME,int&);
    std::shared_ptr<Param> get_param(PARAM_NAME,unsigned int&);
    std::shared_ptr<Param> get_param(PARAM_NAME,std::string&);

    std::shared_ptr<Param> set_param(PARAM_NAME,bool);
    std::shared_ptr<Param> set_param(PARAM_NAME,double);
    std::shared_ptr<Param> set_param(PARAM_NAME,int);
    std::shared_ptr<Param> set_param(PARAM_NAME,unsigned int);
    std::shared_ptr<Param> set_param(PARAM_NAME,const std::string&);
    std::shared_ptr<Param> set_param(PARAM_NAME,const char*);

    std::shared_ptr<Transfer> start_transfer(const std::string& src_filename,
					       const std::string& target_path,
					       bool delete_after_transfer = true);
    std::shared_ptr<CurlLoop::FutureRequest> delete_file(const std::string& filename,
							 bool full_url = false);
    
    void cancel(std::shared_ptr<CurlLoop::FutureRequest> request);
  private:
    std::shared_ptr<Param> _create_get_param(PARAM_NAME);
    template <class T>
    std::shared_ptr<Param> _set_param(PARAM_NAME,const T&);


    typedef std::map<int,std::string> CACHE_TYPE;
    CurlLoop	m_loop;
    CACHE_TYPE	m_cmd_cache_url;
    CACHE_TYPE	m_param_cache_url;
    std::string m_address;
  };
}
