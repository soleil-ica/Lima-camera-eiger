//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : ResourceFactory.h
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
/*! \class     ResourceFactory
 *  \brief     Class used to create the Resource objects
*/
//*****************************************************************************

#ifndef _RESOURCEFACTORY_H
#define _RESOURCEFACTORY_H

#include "Resource.h"
#include "ResourceDescription.h"

#include <string>
#include <map>
#include <list>

#define HTTP_ROOT          "http://"
#define STR_EIGER_API      "api"
#define STR_EIGER_DATA_FILE "lima_data_000001.h5"
#define STR_EIGER_MASTER_FILE "lima_master.h5"

namespace eigerapi
{

typedef std::map <std::string, ResourceDescription*> tResourceMap;

class ResourceFactory
{
public:
    ResourceFactory(const std::string& strIP, const std::string& strAPIVersion);
   ~ResourceFactory();
  
   void setFileNamePattern(const std::string& pattern);   
   Resource* getResource(const std::string& resourceName);
   void getResourceList(std::list<std::string>&);
private:
   ResourceDescription* getDescription(const std::string& resourceName);

   tResourceMap m_resources;

   std::string m_ip_addr;
   std::string m_api_version;
};

} // namespace eigerapi


#endif  //_RESOURCEFACTORY_H
