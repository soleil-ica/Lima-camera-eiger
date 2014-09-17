//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : ResourceFile.cpp
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

#include "ResourceFile.h"
#include "RESTfulClient.h"

#include <iostream>

namespace eigerapi
{
//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
ResourceFile::ResourceFile(const std::string& url) ///< [in] url of the resource
:Resource(url)
{
}


//---------------------------------------------------------------------------
/// Destructor
//---------------------------------------------------------------------------
ResourceFile::~ResourceFile()
{
}


//---------------------------------------------------------------------------
/// Download the file
/*!
@return true if success
 */
//---------------------------------------------------------------------------
bool ResourceFile::download(const std::string& destination) ///< [in] full path with file name where to save
{
   bool bResult = false;

   try
   {
      RESTfulClient client;
      client.get_file(Resource::m_URL, destination);
      bResult = true;
   }
   catch (const EigerException& e)
   {
   	  e.dump();
   }
   return bResult;
}


//---------------------------------------------------------------------------
/// Erase the file
/*!
@return true if success
 */
//---------------------------------------------------------------------------
bool ResourceFile::erase()
{
	bool bResult = false;

	RESTfulClient client;
	try
	{
	 	client.delete_file(Resource::m_URL);
		bResult = true;
	}
	catch (const EigerException& e)
	{
		e.dump();
		bResult = false;
	}
	return bResult;
}

} // namespace eigerapi