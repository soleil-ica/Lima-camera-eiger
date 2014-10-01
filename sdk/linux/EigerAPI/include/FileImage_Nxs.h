//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : CFileImage_Nxs.h
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
/*! \class CFileImage_Nxs
 *  \brief Implements IFileImage using NexusCPP
*/
//*****************************************************************************

#ifndef _CFILEIMAGE_NXS_H
#define _CFILEIMAGE_NXS_H

#include <nexuscpp/nexuscpp.h>

#include "IFileImage.h"

namespace eigerapi
{

class CFileImage_Nxs : public IFileImage
{
public:
    CFileImage_Nxs();
   ~CFileImage_Nxs();

   virtual long openFile(const std::string& fileName);
   virtual void* getNextImage();
   virtual void closeFile();
   
   void GetSize(int& width, int& height, int& depth);

private:
   void ClearData();

   nxcpp::NexusFile* m_nxFile;
   nxcpp::NexusDataSet m_dataSet;
   nxcpp::NexusDataSet* m_pSubsetNextImage;
   nxcpp::NexusDataType m_dataType;
   
   long m_imageIndex;   
   long m_nbImages;
   int  m_width, m_heigth;
   int  m_DatumSize;
};

} // namespace eigerapi


#endif  // _CFILEIMAGE_NXS_H
