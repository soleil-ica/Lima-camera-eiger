//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : ResourceValue.h
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

#ifndef _RESOURCEVALUE_H
#define _RESOURCEVALUE_H


#include "Resource.h"

namespace eigerapi
{

class ResourceValue : public Resource
{
friend class ResourceFactory;

private:
   ResourceValue(const std::string& url, const bool bWritable=false);

public:
   virtual ~ResourceValue();

   template <typename T>
   void get(T &);
    
   template <typename T>
   void set(const T&);
   
private:
   bool m_is_writable;

};

} // namespace eigerapi

///////////////////////////////////////////////////////////////////////////////
//// INCLUDE TEMPLATE IMPLEMENTAION
///////////////////////////////////////////////////////////////////////////////    
#include "ResourceValue.hpp"

#endif  //_RESOURCEVALUE_H
