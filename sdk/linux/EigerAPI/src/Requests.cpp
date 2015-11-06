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
#include <cstdlib>
#include <cstring>
#include <stdio.h>

#include <sstream>

#include <jsoncpp/json/json.h>

#include "eigerapi/Requests.h"
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

static const char* CSTR_EIGERCONFIG		= "config";
static const char* CSTR_EIGERSTATUS		= "status";
static const char* CSTR_EIGERSTATUS_BOARD	= "status/board_000";
static const char* CSTR_EIGERCOMMAND		= "command";
static const char* CSTR_SUBSYSTEMFILEWRITER	= "filewriter";
static const char* CSTR_SUBSYSTEMSTREAM		= "stream";
static const char* CSTR_SUBSYSTEMDETECTOR	= "detector";
static const char* CSTR_DATA			= "data";
static const char* CSTR_EIGERVERSION		= "version";
static const char* CSTR_EIGERAPI		= "api";

struct ResourceDescription
{
   ResourceDescription(const char* name,
                       const char* subsystem=CSTR_SUBSYSTEMDETECTOR,
                       const char* location=CSTR_EIGERCONFIG) :
     m_name(name),
     m_subsystem(subsystem),
     m_location(location)
  {};
  
  std::string build_url(const std::ostringstream& base_url,
			const std::ostringstream& api);
  
  const char*       m_name;
  const char*       m_subsystem;   
  const char*       m_location;
  std::string	    m_url;
};

struct CommandIndex
{
  Requests::COMMAND_NAME	name;
  ResourceDescription		desc;
};

static CommandIndex CommandsDescription[] = {
  {Requests::INITIALIZE,	{"initialize",CSTR_SUBSYSTEMDETECTOR,CSTR_EIGERCOMMAND}},
  {Requests::ARM,		{"arm",CSTR_SUBSYSTEMDETECTOR,CSTR_EIGERCOMMAND}},
  {Requests::DISARM,		{"disarm",CSTR_SUBSYSTEMDETECTOR,CSTR_EIGERCOMMAND}},
  {Requests::TRIGGER,		{"trigger",CSTR_SUBSYSTEMDETECTOR,CSTR_EIGERCOMMAND}},
  {Requests::CANCEL,		{"cancel",CSTR_SUBSYSTEMDETECTOR,CSTR_EIGERCOMMAND}},
  {Requests::ABORT,		{"abort",CSTR_SUBSYSTEMDETECTOR,CSTR_EIGERCOMMAND}},
};

const char* get_cmd_name(Requests::COMMAND_NAME cmd_name)
{
  int nb_cmd = sizeof(CommandsDescription) / sizeof(CommandIndex);
  for(int i = 0;i < nb_cmd;++i)
    {
      if(CommandsDescription[i].name == cmd_name)
	return CommandsDescription[i].desc.m_name;
    }
  return "not found";		// weired
}
struct ParamIndex
{
  Requests::PARAM_NAME	name;
  ResourceDescription	desc;
};

ParamIndex ParamDescription[] = {
  // Detector Read only values
  {Requests::TEMP,			{"th0_temp",CSTR_SUBSYSTEMDETECTOR,CSTR_EIGERSTATUS_BOARD}},
  {Requests::HUMIDITY,			{"th0_humidity",CSTR_SUBSYSTEMDETECTOR,CSTR_EIGERSTATUS_BOARD}},
  {Requests::DETECTOR_STATUS,		{"state",CSTR_SUBSYSTEMDETECTOR,CSTR_EIGERSTATUS}},
  {Requests::PIXELDEPTH,		{"bit_depth_readout"}},
  {Requests::X_PIXEL_SIZE,		{"x_pixel_size"}},
  {Requests::Y_PIXEL_SIZE,		{"y_pixel_size"}},
  {Requests::DETECTOR_WITDH,		{"x_pixels_in_detector"}},
  {Requests::DETECTOR_HEIGHT,		{"y_pixels_in_detector"}},
  {Requests::DESCRIPTION,		{"description"}},
  {Requests::DETECTOR_NUMBER,		{"detector_number"}},
  {Requests::DETECTOR_READOUT_TIME,	{"detector_readout_time"}},
  // Detector Read/Write settings
  {Requests::EXPOSURE,			{"count_time"}},
  {Requests::FRAME_TIME,		{"frame_time"}},
  {Requests::TRIGGER_MODE,		{"trigger_mode"}},
  {Requests::COUNTRATE_CORRECTION,	{"countrate_correction_applied"}},
  {Requests::FLATFIELD_CORRECTION,	{"flatfield_correction_applied"}},
  {Requests::EFFICIENCY_CORRECTION,	{"efficiency_correction_applied"}},
  {Requests::PIXEL_MASK,		{"pixel_mask_applied"}},
  {Requests::THRESHOLD_ENERGY,		{"threshold_energy"}},
  {Requests::VIRTUAL_PIXEL_CORRECTION,	{"virtual_pixel_correction_applied"}},
  {Requests::PHOTON_ENERGY,		{"photon_energy"}},
  {Requests::NIMAGES,			{"nimages"}},
  {Requests::NTRIGGER,			{"ntrigger"}},
  {Requests::AUTO_SUMMATION,		{"auto_summation"}},
  // Filewriter settings
  {Requests::FILEWRITER_MODE,		{"mode",CSTR_SUBSYSTEMFILEWRITER}},
  {Requests::FILEWRITER_COMPRESSION,	{"compression_enabled", CSTR_SUBSYSTEMFILEWRITER}},
  {Requests::FILEWRITER_NAME_PATTERN,	{"name_pattern", CSTR_SUBSYSTEMFILEWRITER}},
  {Requests::NIMAGES_PER_FILE,		{"nimages_per_file", CSTR_SUBSYSTEMFILEWRITER}},
  {Requests::FILEWRITER_STATUS,		{"state", CSTR_SUBSYSTEMFILEWRITER, CSTR_EIGERSTATUS}},
  {Requests::FILEWRITER_ERROR,		{"error", CSTR_SUBSYSTEMFILEWRITER, CSTR_EIGERSTATUS}},
  {Requests::FILEWRITER_TIME,		{"time", CSTR_SUBSYSTEMFILEWRITER, CSTR_EIGERSTATUS}},
  {Requests::FILEWRITER_BUFFER_FREE,	{"buffer_free", CSTR_SUBSYSTEMFILEWRITER, CSTR_EIGERSTATUS}},
  {Requests::FILEWRITER_LS,		{"files", CSTR_SUBSYSTEMFILEWRITER,NULL}},
  {Requests::STREAM_MODE,		{"mode", CSTR_SUBSYSTEMSTREAM}},
  {Requests::STREAM_HEADER_DETAIL,	{"header_detail", CSTR_SUBSYSTEMSTREAM}},
};

const char* get_param_name(Requests::PARAM_NAME param_name)
{
  int nb_param = sizeof(ParamDescription) / sizeof(ParamIndex);
  for(int i = 0;i < nb_param;++i)
    {
      if(ParamDescription[i].name == param_name)
	return ParamDescription[i].desc.m_name;
    }
  return "not found";		// weired
}

std::string ResourceDescription::build_url(const std::ostringstream& base_url,
					   const std::ostringstream& api)
{
  std::ostringstream url;
  if(!m_location)
    url << base_url.str() << m_subsystem << api.str() << m_name;
  else    
    url << base_url.str() << m_subsystem << api.str() << m_location << '/' << m_name;
  return move(url.str());
}

// Requests class
Requests::Requests(const std::string& address) :
  m_address(address)
{
  std::ostringstream base_url;
  base_url << "http://" << address << '/';

  std::ostringstream url;
  url  << base_url.str() << CSTR_SUBSYSTEMDETECTOR << '/'
       << CSTR_EIGERAPI << '/' << CSTR_EIGERVERSION << '/';
  
  std::shared_ptr<Param> version_request(new Param(url.str()));
  version_request->_fill_get_request();
  m_loop.add_request(version_request);
  
  Requests::Param::Value value = version_request->get();
  std::string& api_version = value.string_val;
  
  std::ostringstream api;
  api << '/' << CSTR_EIGERAPI << '/' << api_version << '/';
  
  // COMMANDS URL CACHE
  int nb_cmd = sizeof(CommandsDescription) / sizeof(CommandIndex);
  for(int i = 0;i < nb_cmd;++i)
    {
      CommandIndex& index = CommandsDescription[i];
      m_cmd_cache_url[index.name] = index.desc.build_url(base_url,api);
    }
  // PARAMS URL CACHE
  int nb_params = sizeof(ParamDescription) / sizeof(ParamIndex);
  for(int i = 0;i < nb_params;++i)
    {
      ParamIndex& index = ParamDescription[i];
      m_param_cache_url[index.name] = index.desc.build_url(base_url,api);
    }
}

Requests::~Requests()
{
}

std::shared_ptr<Requests::Command>
Requests::get_command(Requests::COMMAND_NAME cmd_name)
{
  CACHE_TYPE::iterator cmd_url = m_cmd_cache_url.find(cmd_name);
  if(cmd_url == m_cmd_cache_url.end())
    THROW_EIGER_EXCEPTION(RESOURCE_NOT_FOUND,get_cmd_name(cmd_name));

  std::shared_ptr<Requests::Command> cmd(new Command(cmd_url->second));
  cmd->_fill_request();
  m_loop.add_request(cmd);
  return move(cmd);
}

std::shared_ptr<Requests::Param>
Requests::get_param(Requests::PARAM_NAME param_name)
{
  std::shared_ptr<Requests::Param> param = _create_get_param(param_name);

  m_loop.add_request(param);
  return move(param);
}

#define GENERATE_GET_PARAM()						\
  std::shared_ptr<Requests::Param> param = _create_get_param(param_name); \
  param->_set_return_value(ret_value);					\
									\
  m_loop.add_request(param);						\
  return move(param);							\

std::shared_ptr<Requests::Param>
Requests::get_param(Requests::PARAM_NAME param_name,bool& ret_value)
{
  GENERATE_GET_PARAM();
}

std::shared_ptr<Requests::Param>
Requests::get_param(Requests::PARAM_NAME param_name,
		    double& ret_value)
{
  GENERATE_GET_PARAM();
}

std::shared_ptr<Requests::Param>
Requests::get_param(Requests::PARAM_NAME param_name,
		    int& ret_value)
{
  GENERATE_GET_PARAM();
}

std::shared_ptr<Requests::Param>
Requests::get_param(Requests::PARAM_NAME param_name,
		    unsigned int& ret_value)
{
  GENERATE_GET_PARAM();
}

std::shared_ptr<Requests::Param>
Requests::get_param(Requests::PARAM_NAME param_name,
		    std::string& ret_value)
{
  GENERATE_GET_PARAM();
}

std::shared_ptr<Requests::Param>
Requests::_create_get_param(Requests::PARAM_NAME param_name)
{
  CACHE_TYPE::iterator param_url = m_param_cache_url.find(param_name);
  if(param_url == m_param_cache_url.end())
    THROW_EIGER_EXCEPTION(RESOURCE_NOT_FOUND,get_param_name(param_name));
  
  std::shared_ptr<Requests::Param> param(new Param(param_url->second));
  param->_fill_get_request();
  return move(param);
}

template <class T>
std::shared_ptr<Requests::Param>
Requests::_set_param(Requests::PARAM_NAME param_name,const T& value)
{
  CACHE_TYPE::iterator param_url = m_param_cache_url.find(param_name);
  if(param_url == m_param_cache_url.end())
    THROW_EIGER_EXCEPTION(RESOURCE_NOT_FOUND,get_param_name(param_name));

  std::shared_ptr<Requests::Param> param(new Param(param_url->second));
  param->_fill_set_request(value);
  m_loop.add_request(param);
  return move(param);
}

std::shared_ptr<Requests::Param>
Requests::set_param(Requests::PARAM_NAME name,bool value)
{
  return _set_param(name,value);
}

std::shared_ptr<Requests::Param>
Requests::set_param(PARAM_NAME name,double value)
{
  return _set_param(name,value);
}

std::shared_ptr<Requests::Param>
Requests::set_param(PARAM_NAME name,int value)
{
  return _set_param(name,value);
}

std::shared_ptr<Requests::Param>
Requests::set_param(PARAM_NAME name,unsigned int value)
{
  return _set_param(name,value);
}

std::shared_ptr<Requests::Param>
Requests::set_param(PARAM_NAME name,const std::string& value)
{
  return _set_param(name,value);
}

std::shared_ptr<Requests::Param>
Requests::set_param(PARAM_NAME name,const char* value)
{
  return _set_param(name,value);
}


std::shared_ptr<Requests::Transfer> 
Requests::start_transfer(const std::string& src_filename,
			  const std::string& dest_path,
			  bool delete_after_transfer)
{
  std::ostringstream url;
  url << "http://" << m_address << '/' << CSTR_DATA << '/'
      << src_filename;
  
  std::shared_ptr<Transfer> transfer(new Transfer(*this,
						  url.str(),
						  dest_path,
						  delete_after_transfer));
  m_loop.add_request(transfer);
  return move(transfer);
}

std::shared_ptr<CurlLoop::FutureRequest>
Requests::delete_file(const std::string& filename,bool full_url)
{
  std::ostringstream url;
  if(!full_url)
    url << "http://" << m_address << '/' << CSTR_DATA << '/'
	<< filename;
  else
    url << filename;

 std::shared_ptr<CurlLoop::FutureRequest> 
   delete_req(new CurlLoop::FutureRequest(url.str()));
 CURL* handle = delete_req->get_handle();
 curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "DELETE"); 
 m_loop.add_request(delete_req);
 return move(delete_req);
}

void Requests::cancel(std::shared_ptr<CurlLoop::FutureRequest> req)
{
  m_loop.cancel_request(req);
}
//Class Command
Requests::Command::Command(const std::string& url) :
  CurlLoop::FutureRequest(url)
{
  m_data[0] = '\0';
}

Requests::Command::~Command()
{
}

void Requests::Command::_fill_request()
{
  curl_easy_setopt(m_handle,CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(m_handle,CURLOPT_WRITEFUNCTION,_write_callback);
  curl_easy_setopt(m_handle,CURLOPT_WRITEDATA,this);
}

size_t Requests::Command::_write_callback(char *ptr,size_t size,
					  size_t nmemb,Requests::Command *cmd)
{
  int size_to_copy = std::min(size * nmemb,sizeof(m_data));
  memcpy(cmd->m_data,ptr,size_to_copy);
  cmd->m_data[size_to_copy] = '\0';
  return size_to_copy;
}

int Requests::Command::get_serie_id()
{
  Json::Value root;
  Json::Reader reader;
  if(!reader.parse(m_data,root))
    THROW_EIGER_EXCEPTION(eigerapi::JSON_PARSE_FAILED,"");

  int seq_id = root.get("sequence id", -1).asInt();
  return seq_id;
}
//Class Param

Requests::Param::Param(const std::string& url) :
  CurlLoop::FutureRequest(url),
  m_data_buffer(NULL),
  m_data_size(0),
  m_data_memorysize(0),
  m_headers(NULL),
  m_return_value(NULL)
{
}

Requests::Param::~Param()
{
  if(m_data_memorysize)
    free(m_data_buffer);
  
  if(m_headers)
    curl_slist_free_all(m_headers);
}
Requests::Param::Value Requests::Param::get(double timeout,bool lock)
{
  wait(timeout,lock);
  //check rx data
  if(!m_data_buffer)
    THROW_EIGER_EXCEPTION("No data received","");

  if(m_data_size == m_data_memorysize) // need to add an extra char
    {
      int alloc_size = (m_data_memorysize + 1024) & ~1023;
      m_data_buffer = (char*)realloc(m_data_buffer,alloc_size);
      m_data_memorysize = alloc_size;
    }

  m_data_buffer[m_data_size] = '\0'; // string ending

  //- Json decoding to return the wanted data
  Json::Value  root;
  Json::Reader reader;

  if (!reader.parse(m_data_buffer, root)) 
    THROW_EIGER_EXCEPTION(eigerapi::JSON_PARSE_FAILED,"");
  Value value;
  if(root.isArray())
    {
      value.type = Requests::Param::STRING_ARRAY;
      int array_size = root.size();
      for(int i = 0;i < array_size;++i)
	value.string_array.push_back(root[i].asString());
    }
  else
    {
      //- supported types by dectris are:
      //- bool, float, int, string or a list of float or int
      std::string json_type = root.get("value_type", "dummy").asString();
      if (json_type == "bool")
	{
	  value.type = Requests::Param::BOOL;
	  value.data.bool_val = root.get("value", "no_value").asBool();
	}
      else if (json_type == "float")
	{
	  //- asFloat is not supported by jsoncpp
	  value.type = Requests::Param::DOUBLE;
	  value.data.double_val = root.get("value", -1.0).asDouble();
	}
      else if (json_type == "int")
	{
	  value.type = Requests::Param::INT;
	  value.data.int_val = root.get("value", -1).asInt();
	}
      else if (json_type == "uint")
	{
	  value.type = Requests::Param::UNSIGNED;
	  value.data.unsigned_val = (int) root.get("value", -1).asInt();
	}
      else if (json_type == "string")
	{
	  value.type = Requests::Param::STRING;
	  value.string_val = root.get("value","no_value").asString();
	}
      else
	{
	  THROW_EIGER_EXCEPTION(eigerapi::DATA_TYPE_NOT_HANDLED,
				json_type.c_str());
	}
    }
  return value;
}

void Requests::Param::_fill_get_request()
{
  curl_easy_setopt(m_handle,CURLOPT_WRITEFUNCTION,_write_callback);
  curl_easy_setopt(m_handle,CURLOPT_WRITEDATA,this);
}

template <class T>
void Requests::Param::_fill_set_request(const T& value)
{
  Json::Value root;
  root["value"] = value;
  Json::FastWriter writer;
  std::string json_struct = writer.write(root);

  m_headers = curl_slist_append(m_headers, "Accept: application/json");
  m_headers = curl_slist_append(m_headers, "Content-Type: application/json;charset=utf-8");

  curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, m_headers);
  curl_easy_setopt(m_handle, CURLOPT_CUSTOMREQUEST, "PUT"); 

  m_data_buffer = strdup(json_struct.c_str()),m_data_memorysize = json_struct.length();
  curl_easy_setopt(m_handle, CURLOPT_POSTFIELDS, m_data_buffer); // data goes here
  curl_easy_setopt(m_handle, CURLOPT_POSTFIELDSIZE,json_struct.length()); // data length
}

void Requests::Param::_set_return_value(bool& ret_value)
{
  m_return_value = &ret_value;
  m_return_type = BOOL;
}
void Requests::Param::_set_return_value(double& ret_value)
{
  m_return_value = &ret_value;
  m_return_type = DOUBLE;
}
void Requests::Param::_set_return_value(int& ret_value)
{
  m_return_value = &ret_value;
  m_return_type = INT;
}
void Requests::Param::_set_return_value(unsigned int& ret_value)
{
  m_return_value = &ret_value;
  m_return_type = UNSIGNED;
}
void Requests::Param::_set_return_value(std::string& ret_value)
{
  m_return_value = &ret_value;
  m_return_type = STRING;
}

void Requests::Param::_request_finished()
{
  if(m_status == CANCEL) return;

  std::string error_string;
  if(m_return_value)
    {
      Value value;
      try {value = get(0,false);}
      catch(EigerException &e)
	{
	  error_string = e.what();
	  goto error;
	}

      switch(m_return_type)
	{
	case BOOL:
	  {
	    bool *retrun_val = (bool*)m_return_value;
	    switch(value.type)
	      {
	      case BOOL:
		*retrun_val = value.data.bool_val;break;
	      case INT:
		*retrun_val = value.data.int_val;break;
	      case UNSIGNED:
		*retrun_val = value.data.unsigned_val;break;
	      default:
		error_string = "Rx value is not a bool";
		goto error;
	      }
	    break;
	  }
	case DOUBLE:
	  {
	    double *retrun_val = (double*)m_return_value;
	    switch(value.type)
	      {
	      case INT:
		*retrun_val = value.data.int_val;break;
	      case UNSIGNED:
		*retrun_val = value.data.unsigned_val;break;
	      case DOUBLE:
		*retrun_val = value.data.double_val;break;
	      default:
		error_string = "Rx value is not a double";
		goto error;
	      }
	    break;
	  }
	case INT:
	  {
	    int *retrun_val = (int*)m_return_value;
	    switch(value.type)
	      {
	      case INT:
		*retrun_val = value.data.int_val;break;
	      default:
		error_string = "Rx value is not a integer";
		goto error;
	      }
	    break;
	  }
	case UNSIGNED:
	  {
	    unsigned int *retrun_val = (unsigned int*)m_return_value;
	    switch(value.type)
	      {
	      case INT:
		*retrun_val = value.data.int_val;break;
	      case UNSIGNED:
		*retrun_val = value.data.unsigned_val;break;
	      default:
		error_string = "Rx value is not a unsigned integer";
		goto error;
	      }
	    break;
	  }
	case STRING:
	  {
	    std::string *retrun_val = (std::string*)m_return_value;
	    switch(value.type)
	      {
	      case STRING:
		*retrun_val = value.string_val;break;
	      default:
		error_string = "Rx value is not a string";
		goto error;
	      }
	    break;
	  }
	default:
	  error_string = "Value type not yet managed";
	  goto error;
	}
    }
  return;

 error:
  if(m_error_code.empty())
    m_error_code = error_string + "(" + m_url + ")";
  else
    {
      m_error_code += "\n";
      m_error_code += error_string;
    }
  m_status = ERROR;
}

size_t Requests::Param::_write_callback(char *ptr,size_t size,
					size_t nmemb,void *userdata)
{
  Param *t = (Param*)userdata;
  int size_to_copy = size * nmemb;
  if(size_to_copy > 0)
    {
      int request_memory_size = t->m_data_size + size_to_copy;
      if(request_memory_size > t->m_data_memorysize) // realloc
	{
	  int alloc_size = (request_memory_size + 4095) & ~4095;
	  t->m_data_buffer = (char*)realloc(t->m_data_buffer,alloc_size);
	  t->m_data_memorysize = alloc_size;
	}
      memcpy(t->m_data_buffer + t->m_data_size,ptr,size_to_copy);
      t->m_data_size += size_to_copy;
    }
  return size_to_copy;
}

/*----------------------------------------------------------------------------
			   Class Transfer
----------------------------------------------------------------------------*/
Requests::Transfer::Transfer(Requests& requests,
			     const std::string& url,
			     const std::string& target_path,
			     bool delete_after_transfer,
			     int buffer_write_size) :
  CurlLoop::FutureRequest(url),
  m_requests(requests),
  m_delete_after_transfer(delete_after_transfer),
  m_download_size(0)
{
  if(posix_memalign(&m_buffer,4*1024,buffer_write_size))
    THROW_EIGER_EXCEPTION("Can't allocate write buffer memory","");
  m_target_file = fopen(target_path.c_str(),"w+");
  setbuffer(m_target_file,(char*)m_buffer,buffer_write_size);
  if(!m_target_file)
    THROW_EIGER_EXCEPTION("Can't open target file",target_path.c_str());

  curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, _write);
  curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, this);
}
Requests::Transfer::~Transfer()
{
  if(m_target_file)
    fclose(m_target_file);
  free(m_buffer);
}

size_t
Requests::Transfer::_write(void *ptr, size_t size,
			    size_t nmemb, Requests::Transfer *transfer)
{

  FILE *stream = transfer->m_target_file;
  size_t write_size = fwrite(ptr, size, nmemb, stream);

  Lock lock(&transfer->m_lock);
  if(write_size > 0)
    transfer->m_download_size += size * nmemb;

  return write_size;
}

void Requests::Transfer::_request_finished()
{
  fclose(m_target_file);m_target_file = NULL;
  // start new request to delete the file
  if(m_status == FutureRequest::OK &&
     m_delete_after_transfer)
    m_requests.delete_file(m_url,true);
}
