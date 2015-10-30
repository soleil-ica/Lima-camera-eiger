//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : RESTfulClient.h
/// Creation : 2014/07/22
/// Author   : William BOULADOUX
///
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the GNU General Public License as published by the Free Software
/// Foundation; version 2 of the License.
/// 
/// This program is distributed in the hope that it will be useful, but WITHOUT 
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
/// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
///
//*****************************************************************************
//
/*! \class     RESTfulClient
 *  \brief     Implementation of a restful client using curl and Json (based on FL work)
*/
//*****************************************************************************

#ifndef _RESTFULCLIENT_H
#define _RESTFULCLIENT_H

#include <string>
#include <iostream>

#include <curl/curl.h>

#include <json/json.h>

namespace eigerapi
{
#define LOG_STREAM if (RESTfulClient::m_is_verbose) std::cout
class RESTfulClient
{
public:
    RESTfulClient();
   ~RESTfulClient();

   template <typename T>
   void set_parameter(const std::string& url, const T& value);
   
   template <typename T>
   T get_parameter(const std::string& url);
   
   
   void send_command(const std::string& url);
   
   void get_file(const std::string& url, const std::string& targetPath);
   void delete_file(const std::string& url);
   static void set_verbosity(bool is_verbose){m_is_verbose = is_verbose;}
   static bool m_is_verbose;   
private:
    static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
    static size_t write_callback(char* buf, size_t size, size_t nmemb, void* /*up*/);
    static size_t header_callback(char* buffer,   size_t size,   size_t nitems,   void* /*userdata*/);   

   static std::string m_client_data;    
   CURL* m_curl; //our curl object
   
};

} // namespace eigerapi

///////////////////////////////////////////////////////////////////////////////
//// INCLUDE TEMPLATE IMPLEMENTAION
///////////////////////////////////////////////////////////////////////////////    
#include "RESTfulClient.hpp"

#endif  //_RESTFULCLIENT_H
