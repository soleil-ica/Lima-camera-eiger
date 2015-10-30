//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : ResourceValue.hpp
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
/*! \class     ResourceValue
 *  \brief     Implements a resource object used to access basic resource types:
 *  \brief     numeric (integer or double), string or boolean in readonly or readwrite mode.
*/
//*****************************************************************************

#ifndef _RESOURCEVALUE_HPP
#define _RESOURCEVALUE_HPP

#include "RESTfulClient.h"
#include "Resource.h"

namespace eigerapi
{

//---------------------------------------------------------------------------
/// Get the value associated to the resource
//---------------------------------------------------------------------------
template <class T>
void ResourceValue::get(T& value)
{
   RESTfulClient client;
   value = client.get_parameter<T>(Resource::m_url);
}


//---------------------------------------------------------------------------
/// Set a value to the resource
//---------------------------------------------------------------------------
template <class T>
void ResourceValue::set(const T& value)
{
   LOG_STREAM << "ResourceValue::set(" << value << ")" << std::endl;
   if (!m_is_writable) 
   {
      throw EigerException(READONLY_RESOURCE, Resource::m_url.c_str(), "ResourceValue::set");
   } 
   RESTfulClient client;
   client.set_parameter<T>(m_url, value);
}


} // namespace eigerapi


#endif  //_RESOURCEVALUE_HPP
