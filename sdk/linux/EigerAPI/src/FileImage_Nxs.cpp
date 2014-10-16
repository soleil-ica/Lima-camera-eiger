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

#define NXMSG_LEN 256

namespace eigerapi
{

//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
CFileImage_Nxs::CFileImage_Nxs()
{
   m_nxFile            = NULL;
   m_pSubsetNextImage  = NULL;
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
      m_nxFile = new nxcpp::NexusFile();
      m_nxFile->OpenRead(fileName.c_str());
//      m_nxFile->Initialize(); //to ensurte that the target is 3.0.0-SNAPSHOT !!
      std::string Groupname  = EIGER_HDF5_GROUP;
      std::string NeXusclass = EIGER_HDF5_CLASS;
      
      if (m_nxFile->OpenGroup(Groupname.c_str(), NeXusclass.c_str()))
      {
         std::cout << "OpenGroup ok" << std::endl;

         std::cout << "OpenDataSet" << std::endl;
         if (m_nxFile->OpenDataSet(EIGER_HDF5_DATASET))
         {            
            std::cout << "OpenDataSet ok" << std::endl;
            //long nrImage_High = -1; // not needed for now
            //m_nxFile->GetAttribute(EIGER_HDF5_ATTR_NR_IMAGE_H, &nrImage_High);
            
            // Extract dataset info
            nxcpp::NexusDataSetInfo nxDatasetinfo;        
            m_nxFile->GetDataSetInfo(&nxDatasetinfo, EIGER_HDF5_DATASET);
            m_nbImages  = *nxDatasetinfo.DimArray();
            nbImmages   =  m_nbImages;
            m_heigth    = *(nxDatasetinfo.DimArray()+1);      
            m_width     = *(nxDatasetinfo.DimArray()+2);
            m_DatumSize = nxDatasetinfo.DatumSize();
            m_dataType  = nxDatasetinfo.DataType();
            
            std::cout << "TotalDimArray: " << *nxDatasetinfo.TotalDimArray() << std::endl;
            std::cout << "Size:          " << nxDatasetinfo.Size()           << std::endl;         
            std::cout << "DatumSize:     " << nxDatasetinfo.DatumSize()      << std::endl;    
         }
      }
   }
   catch(nxcpp::NexusException& e)
   {  
      char nxMsg[NXMSG_LEN];
      e.GetMsg(nxMsg, NXMSG_LEN);
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
void* CFileImage_Nxs::getNextImage()
{   
   char* addr = NULL;
   
   if (m_imageIndex < m_nbImages)
   {                                                                  
      // init subset description array
      int iDim[3];   // Image geometry
      iDim[0]=1;
      iDim[1]=m_heigth;
      iDim[2]=m_width;

      int iStart[3]; // Start position in dataset
      iStart[0]=m_imageIndex;
      iStart[1]=0;
      iStart[2]=0;
      
      try
      {
         // delete previous datasubset
         if (NULL!=m_pSubsetNextImage) delete m_pSubsetNextImage;

         // Create the dataset to retreive one image
                       // NexusDataSet(NexusDataType eDataType, void *pData, int iRank, int *piDim, int *piStart=NULL);
         m_pSubsetNextImage = new nxcpp::NexusDataSet(m_dataType, NULL,             3,       iDim,       iStart);

         m_nxFile->GetDataSubSet(m_pSubsetNextImage, EIGER_HDF5_DATASET);
         addr = (char*)m_pSubsetNextImage->Data();

      }
      catch(nxcpp::NexusException& e)
      {
         char nxMsg[NXMSG_LEN];
         e.GetMsg(nxMsg, NXMSG_LEN);
         throw EigerException(nxMsg, "", "CFileImage_Nxs::getNextImage" );
      }
      catch (const std::exception& e)
      {
         throw EigerException(e.what(), "", "CFileImage_Nxs::getNextImage" );   
      }

      m_imageIndex++;
   }
   
   return (void*)addr;
}


//---------------------------------------------------------------------------
/// close the currently opened file and release buffer memory
//---------------------------------------------------------------------------
void CFileImage_Nxs::closeFile()
{
   ClearData();
   if (NULL!=m_nxFile)
   {
      m_nxFile->Close();
      delete m_nxFile;
      m_nxFile = NULL;
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
   height = m_heigth;
   depth  = m_DatumSize;
}


//---------------------------------------------------------------------------
/// Clear working data
//---------------------------------------------------------------------------
void CFileImage_Nxs::ClearData()
{
   if (NULL!=m_pSubsetNextImage) 
   {
      delete m_pSubsetNextImage;
      m_pSubsetNextImage = NULL;
   }
   
   m_dataSet.Clear();
   
   m_imageIndex = 0;
   m_nbImages   = 0;
   m_width      = 0;
   m_heigth     = 0;
   m_DatumSize  = 0;
}
}
