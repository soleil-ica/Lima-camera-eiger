//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : Resource.cpp
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


#include "Resource.h"

namespace eigerapi
{
//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
Resource::Resource(const std::string& url)
{
   m_URL = url;
}


//---------------------------------------------------------------------------
/// Destructor
//---------------------------------------------------------------------------
Resource::~Resource()
{
}

}