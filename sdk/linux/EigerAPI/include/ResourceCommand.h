//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : ResourceCommand.h
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
/*! \class     ResourceCommand
 *  \brief     Implements the command type resource
*/
//*****************************************************************************

#ifndef _RESOURCECOMMAND_H
#define _RESOURCECOMMAND_H

#include "Resource.h"

namespace eigerapi
{

class ResourceCommand : public Resource
{
friend class ResourceFactory;

   private:
      ResourceCommand(const std::string& url);   

   public:      
      ~ResourceCommand();
        
      void execute();            
};

} // namespace eigerapi

#endif  //_RESOURCECOMMAND_H
