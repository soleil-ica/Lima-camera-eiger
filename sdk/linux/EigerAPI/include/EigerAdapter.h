//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : EigerAdapter.h
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
/*! \class     EigerAdapter
 *  \brief     This class is an adapter of the Eiger RESTful interface
*/
//*****************************************************************************

#ifndef _EIGERADAPTER_H
#define _EIGERADAPTER_H

#include "EigerDefines.h"
#include "ResourceFactory.h"
#include "IFileImage.h"

#include <vector>
#include <string>

namespace eigerapi
{

class EigerAdapter
{
public:
    EigerAdapter(const std::string& strIP);
   ~EigerAdapter();   

// Detector Commands
   void initialize();
   void arm();
   void disarm();
   void trigger();

// Acquired data managenent
   void setImagesPerFile(const int imagesPerFile);
   void downloadAcquiredFile(const std::string& destination);
   void* getFrame();
   void deleteAcquiredFile();

// Detector Configuration
   void setNbImagesToAcquire(const int);

   void setTriggerMode(const ENUM_TRIGGERMODE);
   ENUM_TRIGGERMODE getTriggerMode();

   void setExposureTime(const double value);
   double getExposureTime();

   double getLatencyTime();
   void setLatencyTime(const double latency);

   ENUM_STATE getState(const ENUM_SUBSYSTEM);
   double getTemperature();
   double getHumidity();
   void setCountrateCorrection(const bool enabled);
   bool getCountrateCorrection();
   void setFlatfieldCorrection(const bool enabled);
   bool getFlatfieldCorrection();
   void setPixelMask(const bool enabled);
   bool getPixelMask();
   void setThresholdEnergy(const double value);
   double getThresholdEnergy();
   void setVirtualPixelCorrection(const bool enabled);
   bool getVirtualPixelCorrection();
   void setEfficiencyCorrection(const bool enabled);
   bool getEfficiencyCorrection();

   void setPhotonEnergy(const double value);
   double getPhotonEnergy();
   int getLastError(std::string& msg);

   bool getCompression(void);
   void setCompression(const bool enabled);

// Detector geometry and miscs
   int getBitDepthReadout();
   EigerSize getPixelSize(void);
   EigerSize getDetectorSize(void);
   std::string getDescription(void);
   std::string getDetectorNumber(void);


private:
   int GetIndex(const std::vector<std::string>& vect,      ///< [in] vector to search into
                const std::string& str);                   ///< [in] string to search for

   double getReadoutTime();
   std::string GetAPIVersion();
                
   ResourceFactory* m_pFactory;   
   
   std::vector<std::string> m_vectTriggerModes;
   std::vector<std::string> m_vectStatusString;
   
   IFileImage* m_fileImage;
   double m_readout_time;
   double m_latency_time;
   double m_exposure_time;
   bool   m_compression;
   std::string m_ipaddr;
};

} // namespace eigerapi

#endif  //_EIGERADAPTER_H
