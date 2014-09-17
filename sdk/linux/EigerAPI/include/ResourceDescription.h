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

#define CSTR_EIGERCONFIG         "config"
#define CSTR_EIGERSTATUS         "status"
#define CSTR_EIGERCOMMAND        "command"
#define CSTR_SUBSYSTEMFILEWRITER "filewriter"
#define CSTR_SUBSYSTEMDETECTOR   "detector"
#define CSTR_DATA                "data"
#define CSTR_EIGERVERSION        "version"

namespace eigerapi
{
   typedef enum 
   {
      ERESOURCETYPE_VALUE_READONLY = 0,
      ERESOURCETYPE_VALUE_READWRITE,
      ERESOURCETYPE_COMMAND,
      ERESOURCETYPE_FILE
   } ENUM_RESOURCETYPE;
   
class ResourceDescription
{
   friend class ResourceFactory;

public:                   
   ResourceDescription(const ENUM_RESOURCETYPE eResourceType,
                       const std::string& name, 
                       const std::string& subsystem=CSTR_SUBSYSTEMDETECTOR,
                       const std::string& location=CSTR_EIGERCONFIG);
                       
   ~ResourceDescription();

private:   
   std::string       m_name;
   ENUM_RESOURCETYPE m_eResourceType;
   std::string       m_subsystem;   
   std::string       m_location;

};

} // namespace eigerapi


#endif  //_RESOURCEDESCRIPTION_H
