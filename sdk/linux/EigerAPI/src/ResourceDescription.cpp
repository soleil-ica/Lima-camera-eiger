//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File      : ResourceDescription.cpp
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

#include "ResourceDescription.h"

namespace eigerapi
{


//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
ResourceDescription::ResourceDescription(const ENUM_RESOURCETYPE eResourceType, ///< [in] resource type
                                         const std::string& name,               ///< [in] resource name                                         
                                         const std::string& subsystem,          ///< [in] subsystem 
                                         const std::string& location)           ///< [in] resource location
{
   m_name          = name;
   m_eResourceType = eResourceType;
   m_subsystem     = subsystem;
   
   switch (eResourceType)
   {
      case ERESOURCETYPE_COMMAND: m_location = CSTR_EIGERCOMMAND; break;
      case ERESOURCETYPE_FILE:    m_location = CSTR_DATA; break;
      default: m_location = location;
   }
}


//---------------------------------------------------------------------------
/// Destructor
//---------------------------------------------------------------------------
ResourceDescription::~ResourceDescription()
{
}
}