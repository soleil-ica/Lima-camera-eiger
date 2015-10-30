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
#include "EigerDefines.h"

namespace eigerapi
{

std::string RESTfulClient::m_client_data = "";
bool RESTfulClient::m_is_verbose = false;


#define HTTP_OK 200

//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
RESTfulClient::RESTfulClient()
{
    LOG_STREAM << "" << std::endl;
    LOG_STREAM << "RESTfulClient::RESTfulClient()" << std::endl;
    curl_global_init(CURL_GLOBAL_ALL);
    m_curl = curl_easy_init();
}


//---------------------------------------------------------------------------
/// Destructor
//---------------------------------------------------------------------------
RESTfulClient::~RESTfulClient()
{
    LOG_STREAM << "" << std::endl;
    LOG_STREAM << "RESTfulClient::~RESTfulClient()" << std::endl;
    curl_easy_cleanup(m_curl);
    curl_global_cleanup();
}


//---------------------------------------------------------------------------
/// Callback for get_file()
//---------------------------------------------------------------------------
size_t RESTfulClient::write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

//---------------------------------------------------------------------------
/// Callback for get_parameter()
//---------------------------------------------------------------------------
size_t RESTfulClient::write_callback(char* buf, size_t size, size_t nmemb, void* /*up*/)
{  //callback must have this declaration
    //buf is a pointer to the data that m_curl has for us
    //size*nmemb is the size of the buffer
    for(unsigned int c = 0; c < size*nmemb; c++)
    {
        m_client_data.push_back(buf[c]);
    }
    return size*nmemb; //tell m_curl how many bytes we handled
}

//---------------------------------------------------------------------------
/// Callback for send_command()
//---------------------------------------------------------------------------
size_t RESTfulClient::header_callback(char* buffer,   size_t size,   size_t nitems,   void* /*userdata*/)
{
    for(unsigned int c = 0; c < size*nitems; c++)
    {
        m_client_data.push_back(buffer[c]);
    }
    return size*nitems; //tell m_curl how many bytes we handled
}


//---------------------------------------------------------------------------
/// Execute a command at a given url
/*!
@return value returned by the command
 */
//---------------------------------------------------------------------------
void RESTfulClient::send_command(const std::string& url)
{
    LOG_STREAM << "" << std::endl;
    LOG_STREAM << "RESTfulClient::send_command(" << url << ")" << std::endl;

    m_client_data = "";

    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");

    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str() );
    curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PUT" );
    curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, this);
    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, header_callback);

    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, "");   // empty body
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, 0); // 0 length   

    CURLcode resultPerform = curl_easy_perform(m_curl);
    unsigned int responseCode;
    CURLcode resultGetInfo = curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode); // Retreive the HTTP response code

    curl_slist_free_all(headers);

    if (resultGetInfo != CURLE_OK)
    {
        throw EigerException(curl_easy_strerror(resultGetInfo), url.c_str(), "RESTfulClient::send_command");
    }
    else
    {
        if (responseCode != HTTP_OK )
        {
            std::string paramMsg = url;
            throw EigerException(BAD_REQUEST, paramMsg.c_str(), "RESTfulClient::send_command");
        }
    }


    if (resultPerform != CURLE_OK)
    {
        throw EigerException(curl_easy_strerror(resultPerform), url.c_str(), "RESTfulClient::send_command");
    }

    //LOG_STREAM << "Command response: " << RESTfulClient_data << std::endl;
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
    LOG_STREAM << "" << std::endl;
    LOG_STREAM << "RESTfulClient::get_parameter(" << url << ")" << std::endl;

    m_client_data = "";

    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, write_callback);
    CURLcode result =   curl_easy_perform(m_curl);

    curl_slist_free_all(headers);

    if (result != CURLE_OK)
    {
        throw EigerException(curl_easy_strerror(result), "", "RESTfulClient::get_parameter");
    }

    if (m_client_data.empty())
    {
        throw EigerException(eigerapi::EMPTY_RESPONSE, "", "RESTfulClient::get_parameter");
    }

    //- Json decoding to return the wanted data
    Json::Value  root;
    Json::Reader reader;

    if (!reader.parse(m_client_data, root))
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
    LOG_STREAM << "" << std::endl;
    LOG_STREAM << "RESTfulClient::get_file(" << url << ", " << targetPath << ")" << std::endl;

    int status = remove(targetPath.c_str());

    FILE *fp = fopen(targetPath.c_str() , "wb");
#define FBUFFER_SIZE 32768
    char fileBuffer[FBUFFER_SIZE];
    setbuf(fp, fileBuffer);

    if (NULL==fp)
    {
        throw EigerException(eigerapi::CREATE_FILE,
                             targetPath.c_str(),
                             "RESTfulClient::get_file");
    }

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, fp);

    CURLcode result = curl_easy_perform(m_curl);

    fflush(fp);
    fclose(fp);

    if (result != CURLE_OK)
    {
        throw EigerException(curl_easy_strerror(result), "", "RESTfulClient::get_file");
    }
}


//---------------------------------------------------------------------------
/// Delete the file at the given url
//---------------------------------------------------------------------------
void RESTfulClient::delete_file(const std::string& url) ///< [in] file url to delete
{
    LOG_STREAM << "" << std::endl;       
    LOG_STREAM << "RESTfulClient::delete_file(" << url << ")" << std::endl;

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    CURLcode result = curl_easy_perform(m_curl);

    if (result != CURLE_OK)
    {
        throw EigerException(curl_easy_strerror(result), "", "RESTfulClient::delete_file");
    }
}


} // namespace eigerapi
