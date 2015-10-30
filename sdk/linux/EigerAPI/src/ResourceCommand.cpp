//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : ResourceCommand.cpp
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

#include "ResourceCommand.h"
#include "RESTfulClient.h"

#include <iostream>

namespace eigerapi
{

//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
ResourceCommand::ResourceCommand(const std::string& url) : 
                    Resource(url)
{
}

//---------------------------------------------------------------------------
/// Destructor
//---------------------------------------------------------------------------
ResourceCommand::~ResourceCommand()
{
}

//---------------------------------------------------------------------------
/// Execute the command
//---------------------------------------------------------------------------
void ResourceCommand::execute()
{  
   RESTfulClient client;
   client.send_command(Resource::m_url);
}

}