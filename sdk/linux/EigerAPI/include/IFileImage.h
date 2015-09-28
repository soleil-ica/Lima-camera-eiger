//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : IFileImage.h
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
/*! \interface IFileImage
 *  \brief     Describes a basic interface to easily get images from the acquired HDF5 data file.
 *  \brief     An implementation of this interface shall be used in the Eiger class.
*/
//*****************************************************************************
/*
*  \remark Structure of the Eiger HDF5 Datafile (example file name "data_000000.h5"):
*  \remark HDF5 "data_000000.h5"
*  \remark {
*  \remark    FILE_CONTENTS
*  \remark    {
*  \remark     group      /
*  \remark     group      /entry
*  \remark     dataset    /entry/data
*  \remark    }
*  \remark }
*/

#ifndef _IFILEIMAGE_H
#define _IFILEIMAGE_H

#include <string>

namespace eigerapi
{
   #define EIGER_HDF5_GROUP           "entry"
   #define EIGER_HDF5_CLASS           "NXEntry"
   #define EIGER_HDF5_DATASET         "data"
   #define EIGER_HDF5_ATTR_NR_IMAGE_H "image_nr_high"

class IFileImage
{
public:
   virtual long openFile(const std::string& fileName) =0;
   virtual void* getNextImage(const unsigned int nbFrames=1) =0;
   virtual void closeFile() =0;
};

} // namespace eigerapi

#endif  //_IFILEIMAGE_H

