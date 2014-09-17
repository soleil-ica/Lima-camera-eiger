//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File      : ResourceFile.h
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
/*! \class     ResourceFile
 *  \brief     Implements the file type resource
*/
//*****************************************************************************

#ifndef _RESSOURCEFILE_H
#define _RESSOURCEFILE_H

#include "Resource.h"

namespace eigerapi
{

class ResourceFile : public Resource
{
friend class ResourceFactory;

private:
   ResourceFile(const std::string& url);   

public:    
   ~ResourceFile();
   
   bool erase();
   bool download(const std::string& destination);   
};

} // namespace eigerapi

#endif  //_RESSOURCEFILE_H
