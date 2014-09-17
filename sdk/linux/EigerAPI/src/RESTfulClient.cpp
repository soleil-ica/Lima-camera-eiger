//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : RESTfulClient.cpp
/// Creation : 2014/09/05
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


#include "RESTfulClient.h"

namespace eigerapi
{

//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
RESTfulClient::RESTfulClient()
{
#ifdef COMPILATION_WITH_CURL
    curl_global_init(CURL_GLOBAL_ALL);
    m_curl = curl_easy_init();
#endif
}


//---------------------------------------------------------------------------
/// Destructor
//---------------------------------------------------------------------------
RESTfulClient::~RESTfulClient()
{
#ifdef COMPILATION_WITH_CURL		
    curl_easy_cleanup(m_curl);
    curl_global_cleanup();
#endif
}

//---------------------------------------------------------------------------
/// Execute a command at a given url
/*!
@return value returned by the command
*/
//---------------------------------------------------------------------------
int RESTfulClient::send_command(const std::string& url)
{
   std::cout << "RESTfulClient::send_command(" << url << ")" << std::endl;
 
#ifdef COMPILATION_WITH_CURL 
   curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str() );
   curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
   curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PUT" );
   curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, this);
   curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, &RESTfulClient_header_callback);

   CURLcode result = curl_easy_perform(m_curl);
   
   if (result != CURLE_OK)
   {
      throw EigerException(curl_easy_strerror(result), "", "RESTfulClient::send_command");     
   }
#endif
   
   int iResult = 0;
   return iResult;
}

//---------------------------------------------------------------------------
/// Get a parameter value at a given url
/*!
@return value of the parameter
*/
//---------------------------------------------------------------------------
template <>
std::string RESTfulClient::get_parameter(const std::string& url)   ///< [in] url to read from
{
   std::cout << "RESTfulClient::get_parameter(" << url << ")" << std::endl;

   RESTfulClient_data = "";

#ifdef COMPILATION_WITH_CURL   
   curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
   curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
   curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &RESTfulClient_writeCallback);   
   CURLcode result =   curl_easy_perform(m_curl);
   if (result != CURLE_OK)
   {
        throw EigerException(curl_easy_strerror(result), "", "RESTfulClient::get_parameter");     
   }
#endif   

   if (RESTfulClient_data.empty())
   {
      throw EigerException(eigerapi::EMPTY_RESPONSE, "", "RESTfulClient::get_parameter");
   }

   //- Json decoding to return the wanted data
   Json::Value  root;
   Json::Reader reader;

   if (!reader.parse(RESTfulClient_data, root)) 
   {
      throw EigerException(eigerapi::JSON_PARSE_FAILED, reader.getFormatedErrorMessages().c_str(),
                           "RESTfulClient::get_parameter");
   }


   //- supported types by dectris are:
   //- bool, float, int, string or a list of float or int  
   std::string value;
   if (root.get("value_type", "dummy").asString() == "string")
   {
         value = root.get("value", "no_value").asString();  
   }
   else
   {
      throw EigerException(eigerapi::DATA_TYPE_NOT_HANDLED, 
                           root.get("value_type", "dummy").asString().c_str(),
                           "RESTfulClient::get_parameter");
   }

   return value;
}

//---------------------------------------------------------------------------
/// Download a file from the given url to the given location
//---------------------------------------------------------------------------
void RESTfulClient::get_file(const std::string& url,        ///< [in] file url to download
                             const std::string& targetPath) ///< [in] target path where to write it
{
   std::cout << "RESTfulClient::get_file(" << url << ", " << targetPath << ")" << std::endl;

#ifdef COMPILATION_WITH_CURL
   FILE *fp = fopen(targetPath.c_str() ,"wb");
   curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
   curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
   curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &RESTfulClient_write_data);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, fp);

   CURLcode result = curl_easy_perform(m_curl);

   fclose(fp);

   if (result != CURLE_OK)
   {
      throw EigerException(curl_easy_strerror(result), "", "RESTfulClient::get_file");     
   }
#endif   
}


//---------------------------------------------------------------------------
/// Delete the file at the given url
//---------------------------------------------------------------------------
void RESTfulClient::delete_file(const std::string& url) ///< [in] file url to delete
{
   std::cout << "RESTfulClient::delete_file(" << url << ")" << std::endl;
   
#ifdef COMPILATION_WITH_CURL
   curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
   curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
   curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE"); 
   
   CURLcode result = curl_easy_perform(m_curl);

   if (result != CURLE_OK)
   {
      throw EigerException(curl_easy_strerror(result), "", "RESTfulClient::delete_file");     
   }
#endif
}


//---------------------------------------------------------------------------
/// Callback for get_parameter()
//---------------------------------------------------------------------------
size_t RESTfulClient_writeCallback(char* buf, size_t size, size_t nmemb, void* /*up*/)
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
size_t RESTfulClient_header_callback(char* /*buffer*/,   size_t size,   size_t nitems,   void* /*userdata*/)
{
   return size*nitems; //tell m_curl how many bytes we handled
}

//---------------------------------------------------------------------------
/// Callback for get_file()
//---------------------------------------------------------------------------
size_t RESTfulClient_write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

} // namespace eigerapi