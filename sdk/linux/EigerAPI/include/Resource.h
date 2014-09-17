//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File      : Resource.h
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
/*! \class     Resource
 *  \brief     Base and abstract class for other resource objects
*/
//*****************************************************************************

#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <string>

namespace eigerapi
{

class Resource
{
friend class ResourceValue;
friend class ResourceCommand;
friend class ResourceFile;

private:
   Resource(const std::string& url);

public:
   virtual ~Resource();

   virtual int execute() { return 0; };
   
   std::string getURL() { return m_URL; };
   
protected:
   std::string m_URL;
};

} // namespace eigerapi


#endif  //_RESOURCE_H
