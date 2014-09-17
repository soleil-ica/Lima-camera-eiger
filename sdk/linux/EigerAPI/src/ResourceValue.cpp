//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : ResourceValue.cpp
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

#ifndef _RESOURCEVALUE_CPP_
#define _RESOURCEVALUE_CPP_

#include "RESTfulClient.h"
#include "ResourceValue.h"

#include <iostream>

namespace eigerapi
{

//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
ResourceValue::ResourceValue(const std::string& url, ///< [in] resource url 
							 const bool bWritable)   ///< [in] true means the value is writeable
///< false means read-only
:Resource(url)
{
	m_bWritable = bWritable;
}

//---------------------------------------------------------------------------
/// Destructor
//---------------------------------------------------------------------------
ResourceValue::~ResourceValue()
{
}

/*template <>
void ResourceValue::get(std::string& value) ///< [out] value of the resource
{
   RESTfulClient client;
   value = client.get_parameter<std::string>(Resource::m_URL);
}
*/

template <>
void ResourceValue::set(const std::string& value) ///< [in] value of the resource
{
   std::cout << "ResourceValue::setString " << value << std::endl;
   if (!m_bWritable) throw ("Resource is read-only.");
   RESTfulClient client;
   client.set_parameter(m_URL, value);
}

}

#endif // _RESOURCEVALUE_CPP_