//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : RESTfulClient.hpp
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


#ifndef _RESTFULCLIENT_HPP_
#define _RESTFULCLIENT_HPP_


#include "EigerDefines.h"


namespace eigerapi
{

//---------------------------------------------------------------------------
/// Set a parameter on the given url
//---------------------------------------------------------------------------
template <class T>
void RESTfulClient::set_parameter(const std::string& url,   ///< [in] URL to write to
                                  const T& value)           ///< [in] value to write
{
   std::cout << "RESTfulClient::set_parameter(" << url << " , " << value << ")" << std::endl;

   std::cout << "preparing json ..." << std::endl<<std::endl;
   Json::Value root;
   root["value"] = value;
   Json::FastWriter writer;
   std::string json_struct = writer.write(root);
   std::cout << "json buffer: " << json_struct << std::endl << std::endl;
   
   //--------------------------
#ifdef COMPILATION_WITH_CURL   
   
   std::cout << "preparing curl ..." << std::endl<<std::endl;
   struct curl_slist *headers = NULL;

   headers = curl_slist_append(headers, "Accept: application/json");
   headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");

   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
   curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);   
   curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
   curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
   curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PUT"); 

   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, json_struct.c_str()); // data goes here
   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, json_struct.length()); // data length

   CURLcode result = curl_easy_perform(m_curl);

   curl_slist_free_all(headers);
   
   if (result != CURLE_OK)
   {
      std::cout << curl_easy_strerror(result) << std::endl;       
      throw EigerException(curl_easy_strerror(result), "", "RESTfulClient::set_parameter");     
   }  
   
//   curl_easy_cleanup(m_curl);
#endif   

   std::cout<<"set_parameter end"<<std::endl;
}


//---------------------------------------------------------------------------
/// Get a parameter value at a given url
/*!
@return value of the parameter
*/
//---------------------------------------------------------------------------
template <class T>
T RESTfulClient::get_parameter(const std::string& url)   ///< [in] url to read from
{
   std::cout << "RESTfulClient::get_parameter(" << url << ")" << std::endl;

   m_RESTfulClient_data = "";
#ifdef COMPILATION_WITH_CURL
   struct curl_slist *headers = NULL;

   headers = curl_slist_append(headers, "Accept: application/json");
   headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");


   curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());

   curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
   curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);   
   curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L); //tell m_curl to output its progress

   CURLcode result =  curl_easy_perform(m_curl);

   curl_slist_free_all(headers);
   
   if (result != CURLE_OK)
   {
        throw EigerException(curl_easy_strerror(result), "", "RESTfulClient::get_parameter");     
   }   
   

   //- Json decoding to return the wanted data
   Json::Value  root;
   Json::Reader reader;

   //std::cout << "m_RESTfulClient_data = " << m_RESTfulClient_data << std::endl;
   if (!reader.parse(m_RESTfulClient_data, root)) 
   {
      throw EigerException(eigerapi::JSON_PARSE_FAILED, reader.getFormatedErrorMessages().c_str(),
                           "RESTfulClient::get_parameter");
   }

   //- supported types by dectris are:
   //- bool, float, int, string or a list of float or int

   T value;
   if (root.get("value_type", "dummy").asString() == "bool")
   {
      value = root.get("value", "no_value").asBool();
   }
   else if (root.get("value_type", "dummy").asString() == "float")
   {
      value = root.get("value", -1.0).asDouble(); //- asFloat is not supported by jsoncpp
   }
   else if (root.get("value_type", "dummy").asString() == "int")
   {
      value = (int) root.get("value", -1).asInt();
   }
   else if (root.get("value_type", "dummy").asString() == "uint")
   {
      value = (int) root.get("value", -1).asInt();
   }
   else
   {
      throw EigerException(eigerapi::DATA_TYPE_NOT_HANDLED, 
                           root.get("value_type", "dummy").asString().c_str(),
                           "RESTfulClient::get_parameter");
   }

   return value;
#else
   T value = T();
   return value;
#endif   

}



} // namespace eigerapi




#endif // _RESTFULCLIENT_HPP_



