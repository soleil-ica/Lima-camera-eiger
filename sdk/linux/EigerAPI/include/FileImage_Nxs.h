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
   virtual void* getNextImage(const unsigned int nbFrames=1);
   virtual void closeFile();
   
   void GetSize(int& width, int& height, int& depth);

private:
   void ClearData();

   nxcpp::NexusFile* m_nxs_file;
   nxcpp::NexusDataSet m_dataset;
   nxcpp::NexusDataSet* m_subset_next_image;
   nxcpp::NexusDataType m_data_type;
   
   long m_image_index;   
   long m_nb_images;
   int  m_width, m_height;
   int  m_datum_size;
};

} // namespace eigerapi


#endif  // _CFILEIMAGE_NXS_H
