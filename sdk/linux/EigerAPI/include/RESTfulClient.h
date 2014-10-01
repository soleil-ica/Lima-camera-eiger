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

#ifdef COMPILATION_WITH_CURL
#include <curl/curl.h>
#endif

#include <json/json.h>

namespace eigerapi
{

static std::string RESTfulClient_data;

//// size_t RESTfulClient_writeCallback(char* buf, size_t size, size_t nmemb, void* /*up*/);
////size_t RESTfulClient_header_callback(char* /*buffer*/,   size_t size,   size_t nitems,   void* /*userdata*/);
////size_t RESTfulClient_write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);


//---------------------------------------------------------------------------
/// Callback for get_parameter()
//---------------------------------------------------------------------------
static size_t RESTfulClient_writeCallback(char* buf, size_t size, size_t nmemb, void* /*up*/)
{  //callback must have this declaration
   //buf is a pointer to the data that m_curl has for us
   //size*nmemb is the size of the buffer
   for(unsigned int c = 0; c < size*nmemb; c++)
   {
      RESTfulClient_data.push_back(buf[c]);
   }
   return size*nmemb; //tell m_curl how many bytes we handled
}

//---------------------------------------------------------------------------
/// Callback for send_command()
//---------------------------------------------------------------------------
static size_t RESTfulClient_header_callback(char* buffer,   size_t size,   size_t nitems,   void* /*userdata*/)
{
   for(unsigned int c = 0; c < size*nitems; c++)
   {
      RESTfulClient_data.push_back(buffer[c]);
   }
   return size*nitems; //tell m_curl how many bytes we handled
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
class RESTfulClient
{
public:
    RESTfulClient();
   ~RESTfulClient();

   template <typename T>
   void set_parameter(const std::string& url, const T& value);
   
   template <typename T>
   T get_parameter(const std::string& url);
   
   
   int send_command(const std::string& url);
   
   void get_file(const std::string& url, const std::string& targetPath);
   void delete_file(const std::string& url);

private:
    static size_t RESTfulClient_write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
    
#ifdef COMPILATION_WITH_CURL    
   CURL* m_curl; //our curl object
#endif   
   
};

} // namespace eigerapi

///////////////////////////////////////////////////////////////////////////////
//// INCLUDE TEMPLATE IMPLEMENTAION
///////////////////////////////////////////////////////////////////////////////    
#include "RESTfulClient.hpp"

#endif  //_RESTFULCLIENT_H
