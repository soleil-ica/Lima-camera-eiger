//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File      : ResourceDescription.h
/// Creation  : 2014/07/22
/// Author    : William BOULADOUX
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
/*! \class     ResourceDescription
 *  \brief     Basic class to store the description of a resource
*/
//*****************************************************************************

#ifndef _RESOURCEDESCRIPTION_H
#define _RESOURCEDESCRIPTION_H

#include <string>

#define STR_EIGER_CONFIG         "config"
#define STR_EIGER_STATUS         "status"
#define STR_EIGER_COMMAND        "command"
#define STR_SUBSYSTEM_FILE_WRITER "filewriter"
#define STR_SUBSYSTEM_DETECTOR   "detector"
#define STR_DATA                "data"
#define STR_EIGER_VERSION        "version"

namespace eigerapi
{
   typedef enum 
   {
      RESOURCE_VALUE_RO = 0,
      RESOURCE_VALUE_RW,
      RESOURCE_COMMAND,
      RESOURCE_FILE
   } ENUM_RESOURCE_TYPE;
   
class ResourceDescription
{
   friend class ResourceFactory;

public:                   
   ResourceDescription(const ENUM_RESOURCE_TYPE eResourceType,
                       const std::string& name, 
                       const std::string& subsystem=STR_SUBSYSTEM_DETECTOR,
                       const std::string& location=STR_EIGER_CONFIG);
                       
   ~ResourceDescription();

private:   
   std::string       m_name;
   ENUM_RESOURCE_TYPE     m_resource_type;
   std::string       m_subsystem;   
   std::string       m_location;

};

} // namespace eigerapi


#endif  //_RESOURCEDESCRIPTION_H
