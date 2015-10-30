//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : CFileImage_Nxs.cpp
/// Creation : 2014/07/28
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

#include "FileImage_Nxs.h"
#include "EigerDefines.h"

#define NXMSG_LEN 1024

namespace eigerapi
{

//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
CFileImage_Nxs::CFileImage_Nxs()
{
   m_nxs_file            = NULL;
   m_subset_next_image  = NULL;
   ClearData();
}


//---------------------------------------------------------------------------
/// Destructor
//---------------------------------------------------------------------------
CFileImage_Nxs::~CFileImage_Nxs()
{
   closeFile();
}


//---------------------------------------------------------------------------
/// Open the given file for reading
/*!
@return number of images contained in the file or -1 if error
*/
//---------------------------------------------------------------------------
long CFileImage_Nxs::openFile(const std::string& fileName) ///< [in] name of the file
{
   long nbImmages = -1;
   
   closeFile(); // in case another file is already open
   
   try
   {
      m_nxs_file = new nxcpp::NexusFile();
      m_nxs_file->OpenRead(fileName.c_str());
      std::string Groupname  = EIGER_HDF5_GROUP;
      std::string NeXusclass = EIGER_HDF5_CLASS;
	  std::string Groupname_next  = EIGER_HDF5_GROUP_NEXT;
      std::string NeXusclass_next = EIGER_HDF5_CLASS_NEXT;
      
      if (m_nxs_file->OpenGroup(Groupname.c_str(), NeXusclass.c_str()))
      {
		if (m_nxs_file->OpenGroup(Groupname_next.c_str(), NeXusclass_next.c_str()))
      	{

         if (m_nxs_file->OpenDataSet(EIGER_HDF5_DATASET))
         {            
            // Extract dataset info
            nxcpp::NexusDataSetInfo nxDatasetinfo;        
            m_nxs_file->GetDataSetInfo(&nxDatasetinfo, EIGER_HDF5_DATASET);
            m_nb_images  = *nxDatasetinfo.DimArray();
            nbImmages   =  m_nb_images;
            m_height    = *(nxDatasetinfo.DimArray()+1);      
            m_width     = *(nxDatasetinfo.DimArray()+2);
            m_datum_size = nxDatasetinfo.DatumSize();
            m_data_type  = nxDatasetinfo.DataType();
         }
		}
      }
   }
   catch(nxcpp::NexusException& e)
   {  
      char nxMsg[NXMSG_LEN];
      e.GetMsg(nxMsg, NXMSG_LEN-1);
      throw EigerException(nxMsg, "", "CFileImage_Nxs::openFile" );
   }
   catch (const std::exception& e)
   {
      throw EigerException(e.what(), "", "CFileImage_Nxs::openFile" );   
   }

   return nbImmages;
}                


//---------------------------------------------------------------------------
/// returns a pointer to the next image data contained in the file
/*!
@return an valid image buffer or NULL if no more images are available
@remark memory allocation is handled by CFileImage (any allocated memory will be released in closeFile())
*/
//---------------------------------------------------------------------------
void* CFileImage_Nxs::getNextImage(const unsigned int nbFrames) /// [in] number of frames to retreive at a time.
{   
   char* addr = NULL;
   
   if (m_image_index < m_nb_images)
   {                                                                  
      // init subset description array
      int iDim[3];   // Image geometry
      iDim[0]=nbFrames;
      iDim[1]=m_height;
      iDim[2]=m_width;

      int iStart[3]; // Start position in dataset
      iStart[0]=m_image_index;
      iStart[1]=0;
      iStart[2]=0;
      
      try
      {
         // delete previous datasubset
         if (NULL!=m_subset_next_image) delete m_subset_next_image;

         // Create the dataset to retreive one image
                       // NexusDataSet(NexusDataType eDataType, void *pData, int iRank, int *piDim, int *piStart=NULL);
         m_subset_next_image = new nxcpp::NexusDataSet(m_data_type, NULL,             3,       iDim,       iStart);

         m_nxs_file->GetDataSubSet(m_subset_next_image, EIGER_HDF5_DATASET);
         addr = (char*)m_subset_next_image->Data();

      }
      catch(nxcpp::NexusException& e)
      {
         char nxMsg[NXMSG_LEN];
         e.GetMsg(nxMsg, NXMSG_LEN-1);
         throw EigerException(nxMsg, "", "CFileImage_Nxs::getNextImage" );
      }
      catch (const std::exception& e)
      {
         throw EigerException(e.what(), "", "CFileImage_Nxs::getNextImage" );   
      }

      m_image_index += nbFrames;
   }
   
   return (void*)addr;
}


//---------------------------------------------------------------------------
/// close the currently opened file and release buffer memory
//---------------------------------------------------------------------------
void CFileImage_Nxs::closeFile()
{
   ClearData();
   if (NULL!=m_nxs_file)
   {
      m_nxs_file->Close();
      delete m_nxs_file;
      m_nxs_file = NULL;
   }   
}


//---------------------------------------------------------------------------
/// Get width, height and pixel depth of images in the open file
//---------------------------------------------------------------------------
void CFileImage_Nxs::GetSize(int& width,   ///< [out] images width
                             int& height,  ///< [out] images height
                             int& depth)   ///< [out] images pixel depth
{
   width  = m_width;
   height = m_height;
   depth  = m_datum_size;
}


//---------------------------------------------------------------------------
/// Clear working data
//---------------------------------------------------------------------------
void CFileImage_Nxs::ClearData()
{
   if (NULL!=m_subset_next_image) 
   {
      delete m_subset_next_image;
      m_subset_next_image = NULL;
   }
   
   m_dataset.Clear();
   
   m_image_index = 0;
   m_nb_images   = 0;
   m_width      = 0;
   m_height     = 0;
   m_datum_size  = 0;
}
}
