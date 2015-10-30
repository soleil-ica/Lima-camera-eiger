//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : EigerAdapter.cpp
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



#include <iostream>


#include "EigerAdapter.h"

#include "FileImage_Nxs.h"
#include "ResourceCommand.h"
#include "RESTfulClient.h"
#include "ResourceFile.h"
#include "ResourceValue.h"

namespace eigerapi
{

//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
EigerAdapter::EigerAdapter(const std::string& strIP)  ///< [in] version of the api on the server (ex: "0.6.1")
{
    LOG_STREAM << "create EigerAPI, IP= " << strIP << std::endl;

    m_ip_addr = strIP;

    // Request the version of the API installed on the server
    std::string strAPIVersion = GetAPIVersion();
    LOG_STREAM << "API version = " << strAPIVersion << std::endl;

    m_factory = new ResourceFactory(strIP, strAPIVersion);

    // Detector trigger modes
	m_vec_trigger_mode.push_back("ints"); // internal single
    m_vec_trigger_mode.push_back("exts"); // external single
    m_vec_trigger_mode.push_back("exte"); // external gate (number of image should be equal to the number of trigger)

    // Subsystems status strings
    m_vec_status.push_back("na");         // ESTATE_NA
    m_vec_status.push_back("disabled");   // ESTATE_DISABLED
    m_vec_status.push_back("ready");      // ESTATE_READY
    m_vec_status.push_back("idle");       // ESTATE_IDLE
    m_vec_status.push_back("acquire");    // ESTATE_ACQUIRE
    m_vec_status.push_back("error");      // ESTATE_ERROR
    m_vec_status.push_back("initialize"); // ESTATE_INITIALIZE
    m_vec_status.push_back("configure");  // ESTATE_CONFIGURE
    m_vec_status.push_back("test");       // ESTATE_TEST

    m_file_image = NULL;
    
    // Send an "initialize" command
    // FL: no need to initialize for each start of the device (only for each start of the detector)
	//initialize();

    // Init capture timings
    m_readout_time = getReadoutTime(); // get the current readout time in the detector. (this value could be improved with firmware update)
    m_exposure_time = 0.0; 
    m_latency_time  = 0.0;  
    LOG_STREAM <<"m_readout_time = "<<m_readout_time<<std::endl;        
    
    
    // Set LZ4 compression enabled by default
    std::shared_ptr<ResourceValue> compression (dynamic_cast<ResourceValue*> (m_factory->getResource("compression")));
    compression->set(true);
    compression->get(m_compression); // store initial value
    LOG_STREAM<<"m_compression = "<<m_compression<<std::endl;
    // Disable auto_summation for now
    std::shared_ptr<ResourceValue> autosummation (dynamic_cast<ResourceValue*> (m_factory->getResource("auto_summation")));
    autosummation->set(false);
    m_auto_summation = false;
    ////autosummation->get(m_auto_summation);
    LOG_STREAM<<"m_auto_summation = "<<m_auto_summation<<std::endl;
}


//---------------------------------------------------------------------------
/// Destructor
//---------------------------------------------------------------------------
EigerAdapter::~EigerAdapter()
{
    //delete Factory object
    delete m_factory;
    m_factory = NULL;
    
    //delete fileImage object
    delete m_file_image;
    m_file_image = NULL;
}


//---------------------------------------------------------------------------
/// Run the initialize command
//---------------------------------------------------------------------------
void EigerAdapter::initialize()
{
    std::shared_ptr<ResourceCommand> initCmd (dynamic_cast<ResourceCommand*> (m_factory->getResource("initialize")));  
    initCmd->execute();
}


//---------------------------------------------------------------------------
/// Run the arm command
//---------------------------------------------------------------------------
void EigerAdapter::arm()
{    
    std::shared_ptr<Resource> armCmd (m_factory->getResource("arm"));
    armCmd->execute();
}


//---------------------------------------------------------------------------
/// Run the disarm command
//---------------------------------------------------------------------------
void EigerAdapter::disarm()
{
    std::shared_ptr<Resource> disarmCmd (m_factory->getResource("disarm"));
    disarmCmd->execute();
}

//---------------------------------------------------------------------------
/// Run the abort command
//---------------------------------------------------------------------------
void EigerAdapter::abort()
{
    std::shared_ptr<Resource> abortCmd (m_factory->getResource("abort"));
    abortCmd->execute();
}

//---------------------------------------------------------------------------
/// Run the cancel command
//---------------------------------------------------------------------------
void EigerAdapter::cancel()
{
    std::shared_ptr<Resource> cancelCmd (m_factory->getResource("cancel"));
    cancelCmd->execute();
}

//---------------------------------------------------------------------------
/// Run the trigger command
//---------------------------------------------------------------------------
void EigerAdapter::trigger()
{
    std::shared_ptr<Resource> triggerCmd (m_factory->getResource("trigger"));
    triggerCmd->execute();
}


//---------------------------------------------------------------------------
/// Run the status_update command
//---------------------------------------------------------------------------
void EigerAdapter::status_update()
{
    std::shared_ptr<Resource> statusUpdateCmd (m_factory->getResource("status_update"));
    statusUpdateCmd->execute();
}

//---------------------------------------------------------------------------
/// set name_pattern
//---------------------------------------------------------------------------
void EigerAdapter::setFileNamePattern(const std::string& pattern) 
{
//    std::shared_ptr<ResourceValue> name_pattern (dynamic_cast<ResourceValue*> (m_factory->getResource("name_pattern")));
//    name_pattern->set(pattern);
    m_factory->setFileNamePattern(pattern);    
} 
    
//---------------------------------------------------------------------------
/// Download the last acquired data file
//---------------------------------------------------------------------------
void EigerAdapter::downloadDataFile(const std::string& destination) ///< [in] full destination path+name where to download the data file                                                                        
{
    std::shared_ptr<ResourceFile> serverFile (dynamic_cast<ResourceFile*> (m_factory->getResource("datafile")));

    serverFile->download(destination);
}

//---------------------------------------------------------------------------
/// Delete the last acquired data file
//---------------------------------------------------------------------------
void EigerAdapter::deleteDataFile()
{
    std::shared_ptr<ResourceFile> resFile (dynamic_cast<ResourceFile*> (m_factory->getResource("datafile")));
    resFile->erase();
}

//---------------------------------------------------------------------------
///(TANGODEVIC-1256)
/// Download the master file
//---------------------------------------------------------------------------
void EigerAdapter::downloadMasterFile(const std::string& destination) ///< [in] full destination path+name where to download the master file                                                                      
{
    std::shared_ptr<ResourceFile> serverFile (dynamic_cast<ResourceFile*> (m_factory->getResource("masterfile")));

    serverFile->download(destination);
}

//---------------------------------------------------------------------------
///(TANGODEVIC-1256)
/// Delete the master file
//---------------------------------------------------------------------------
void EigerAdapter::deleteMasterFile()
{
    std::shared_ptr<ResourceFile> resFile (dynamic_cast<ResourceFile*> (m_factory->getResource("masterfile")));
    resFile->erase();
}

//---------------------------------------------------------------------------
///Open the data file (must be refactored in the future !!)
//---------------------------------------------------------------------------
void EigerAdapter::openDataFile(const std::string& source) ///< [in] the data file name to open
{
    //delete previous object
    delete m_file_image;
    m_file_image = NULL;

    //instantiate a new fileImage object
    std::shared_ptr<ResourceFile> serverFile (dynamic_cast<ResourceFile*> (m_factory->getResource("datafile")));
    m_file_image = new CFileImage_Nxs();        
    m_file_image->openFile(source + std::string("/") + serverFile->getFileName());
}

//---------------------------------------------------------------------------
/// Get a frame from the last opened data file
/*!
@return pointer to valid image data buffer (valid until next call to getFrame() or deleteDataFile())
 */
//---------------------------------------------------------------------------
void* EigerAdapter::getFrame()
{
    if (NULL != m_file_image)
    {
        return m_file_image->getNextImage();
    }
    else
    {
        return NULL;
    }
}

//---------------------------------------------------------------------------
/// Sets the trigger mode on the detector
//---------------------------------------------------------------------------
void EigerAdapter::setTriggerMode(const ENUM_TRIGGERMODE eTrigMode)   ///< [in] trigger mode
{
    std::shared_ptr<ResourceValue> resValue (dynamic_cast<ResourceValue*> (m_factory->getResource("trigger_mode")));
    resValue->set(m_vec_trigger_mode.at(int(eTrigMode)));
}


//---------------------------------------------------------------------------
/// Get the current trigger mode on the detector
/*!
@return enum value of the current trigger mode or ETRIGMODE_UNKNOWN if not found
@remark see ENUM_TRIGGERMODE definition
 */
//---------------------------------------------------------------------------
ENUM_TRIGGERMODE EigerAdapter::getTriggerMode()
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("trigger_mode")));
    std::string strTriggerMode;
    value->get(strTriggerMode);

    // search the index of the trigger mode
    return ENUM_TRIGGERMODE(GetIndex(m_vec_trigger_mode, strTriggerMode));
}


//---------------------------------------------------------------------------
/// Sets the exposure time duration mode on the detector
//---------------------------------------------------------------------------
void EigerAdapter::setExposureTime(const double exposureTime) ///< [in] exposure time value to set
{
    LOG_STREAM << "EigerAdapter::setExposureTime("<<exposureTime<<")"<<std::endl;     
    // set exposure time (Eiger API "count_time" )       
    std::shared_ptr<ResourceValue> valueExpo (dynamic_cast<ResourceValue*> (m_factory->getResource("exposure")));
    valueExpo->set(exposureTime);
    m_exposure_time = exposureTime;

    // Update frame time accordingly (frame_time = exposure + readout +latency )    
    std::shared_ptr<ResourceValue> valueFrameTime (dynamic_cast<ResourceValue*> (m_factory->getResource("frame_time")));
    valueFrameTime->set(m_exposure_time + m_readout_time + m_latency_time);
}


//---------------------------------------------------------------------------
/// Get the current value of the exposure time on the detector
/*!
@return exposure time
 */
//---------------------------------------------------------------------------
double EigerAdapter::getExposureTime()
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("exposure")));
    double exposureTime = 0.0;
    value->get(exposureTime);
    return exposureTime;
}


//---------------------------------------------------------------------------
/// Get the value of the readout time on the detector
/*!
@return exposure time
 */
//---------------------------------------------------------------------------
double EigerAdapter::getReadoutTime()
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("detector_readout_time")));
    double readoutTime = 0.0;
    value->get(readoutTime);

    return readoutTime;
}


//---------------------------------------------------------------------------
/// Get the last set latency time value
/*!
@return latency time
 */
//---------------------------------------------------------------------------
double EigerAdapter::getLatencyTime()
{
    return m_latency_time;
}


//---------------------------------------------------------------------------
/// Set the latency time value
//---------------------------------------------------------------------------
void EigerAdapter::setLatencyTime(const double latency)
{
    LOG_STREAM << "EigerAdapter::setLatencyTime("<<latency<<")"<<std::endl;    
    m_latency_time = latency;
}


//---------------------------------------------------------------------------
/// Get the current state of the detector
/*!
@return enum value of the current state of the detector
@remark possible detector states are:
#remark ESTATE_UNKNOWN
@remark ESTATE_DISABLED
@remark ESTATE_READY
@remark ESTATE_ACQUIRE
@remark ESTATE_ERROR
 */
//---------------------------------------------------------------------------
ENUM_STATE EigerAdapter::getState(const ENUM_SUBSYSTEM eDevice) ///< [in] subsystem to get the state from
{
    std::string resourceName;
    switch (eDevice)
    {
        case SUBSYSTEM_DETECTOR:   resourceName = "detector_status"; break;
        case SUBSYSTEM_FILEWRITER: resourceName = "filewriter_status"; break;
        default: resourceName = "unknown_subsystem";
    }

    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource(resourceName)));
    std::string strStatus;
    value->get(strStatus);
    LOG_STREAM << " " << resourceName << " : " << strStatus << std::endl;

    // Return the index of the status string
    return ENUM_STATE(GetIndex(m_vec_status, strStatus));
}


//---------------------------------------------------------------------------
/// Get the current temperature of the detector
/*!
@return temperature value
 */
//---------------------------------------------------------------------------
double EigerAdapter::getTemperature()
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("temp")));
    double temperature = 0.0;
    value->get(temperature);

    return temperature;
}


//---------------------------------------------------------------------------
/// Get the current humidity level of the detector
/*!
@return humidity value
 */
//---------------------------------------------------------------------------
double EigerAdapter::getHumidity()
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("humidity")));
    double humidity = 0.0;
    value->get(humidity);

    return humidity;
}


//---------------------------------------------------------------------------
// Get the bit depth of the detector (bits per pixel)
/*!
@return bit per pixel value (usually 12 for Eiger 1M)
 */
//---------------------------------------------------------------------------
int EigerAdapter::getBitDepthReadout()
{
    int pixelDepth = 0;
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("pixeldepth")));
    value->get(pixelDepth);
    return pixelDepth;
}

//---------------------------------------------------------------------------
// Enable or disable the count rate correction feature
//---------------------------------------------------------------------------
void EigerAdapter::setCountrateCorrection(const bool enabled) ///< [in] true:enabled, false:disabled
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("countrate_correction")));
    value->set(enabled);
}


//---------------------------------------------------------------------------
// // Get the status of the count rate correction feature
/*!
@return true:enabled, false:disabled
 */
//---------------------------------------------------------------------------
bool EigerAdapter::getCountrateCorrection()
{
    bool countrateCorrection = false;
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("countrate_correction")));
    value->get(countrateCorrection);

    return countrateCorrection;
}


//---------------------------------------------------------------------------
// Enable or disable the flat field correction feature
//---------------------------------------------------------------------------
void EigerAdapter::setFlatfieldCorrection(const bool enabled) ///< [in] true:enabled, false:disabled
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("flatfield_correction")));
    value->set(enabled);
}


//---------------------------------------------------------------------------
// Get the status of the Flat field Correction feature
/*!
@return true:enabled, false:disabled
 */
//---------------------------------------------------------------------------
bool EigerAdapter::getFlatfieldCorrection()
{
    bool flatfieldCorrection = false;
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("flatfield_correction")));
    value->get(flatfieldCorrection);

    return flatfieldCorrection;
}


//---------------------------------------------------------------------------
// Set the status of the pixel mask feature
//---------------------------------------------------------------------------
void EigerAdapter::setPixelMask(const bool enabled) ///< [in] true:enabled, false:disabled
{
    LOG_STREAM << "EigerAdapter::setPixelMask " << enabled << std::endl;

    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("pixel_mask")));
    value->set(enabled);
}


//---------------------------------------------------------------------------
// Get the status of the pixel mask feature
/*!
@return true:enabled, false:disabled
 */
//---------------------------------------------------------------------------
bool EigerAdapter::getPixelMask()
{
    bool pixelMask = false;
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("pixel_mask")));
    value->get(pixelMask);

    return pixelMask;
}


//---------------------------------------------------------------------------
// Set the value of ThresholdEnergy
//---------------------------------------------------------------------------
void EigerAdapter::setThresholdEnergy(const double thresholdEnergy) ///< [in] energy value
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("threshold_energy")));
    value->set(thresholdEnergy);
}


//---------------------------------------------------------------------------
// Get the value of ThresholdEnergy
/*!
@return ThresholdEnergy value
 */
//---------------------------------------------------------------------------
double EigerAdapter::getThresholdEnergy()
{
    double thresholdEnergy = 0.0;
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("threshold_energy")));
    value->get(thresholdEnergy);

    return thresholdEnergy;
}


//---------------------------------------------------------------------------
// Set the status of the VirtualPixelCorrection feature
//---------------------------------------------------------------------------
void EigerAdapter::setVirtualPixelCorrection(const bool enabled) ///< [in] true:enabled, false:disabled
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("virtual_pixel_correction")));
    value->set(enabled);
}


//---------------------------------------------------------------------------
// Get the status of the VirtualPixelCorrection feature
/*!
@return true:enabled, false:disabled
 */
//---------------------------------------------------------------------------
bool EigerAdapter::getVirtualPixelCorrection()
{
    bool virtualPixelCorrection = false;
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("virtual_pixel_correction")));
    value->get(virtualPixelCorrection);

    return virtualPixelCorrection;
}


//---------------------------------------------------------------------------
// Set the status of the VirtualPixelCorrection feature
//---------------------------------------------------------------------------
void EigerAdapter::setEfficiencyCorrection(const bool enabled)
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("efficiency_correction")));
    value->set(enabled);
}


//---------------------------------------------------------------------------
// Get the status of the VirtualPixelCorrection feature
/*!
@return true:enabled, false:disabled
 */
//---------------------------------------------------------------------------   
bool EigerAdapter::getEfficiencyCorrection()
{
    bool efficiencyCorrection = false;
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("efficiency_correction")));
    value->get(efficiencyCorrection);

    return efficiencyCorrection;
}

//---------------------------------------------------------------------------
// Set the photon energy value
//---------------------------------------------------------------------------
void EigerAdapter::setPhotonEnergy(const double photonEnergy) ///< [in] energy value
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("photon_energy")));
    value->set(photonEnergy);
}


//---------------------------------------------------------------------------
// Get the photonenergy value
/*!
@return photon energy value
 */
//---------------------------------------------------------------------------
double EigerAdapter::getPhotonEnergy()
{
    double photonEnergy = 0.0;
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("photon_energy")));
    value->get(photonEnergy);

    return photonEnergy;
}


//---------------------------------------------------------------------------
// Get the last error message (when an error has occured)
//---------------------------------------------------------------------------
int EigerAdapter::getLastError(std::string& msg) ///< [out] error message
{
    msg = "";
    return -1;
}


//---------------------------------------------------------------------------
// Get the detector description string
/*!
@return detector description string
 */
//---------------------------------------------------------------------------
std::string EigerAdapter::getDescription()
{
    std::string descriptionStr;

    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("description")));
    value->get(descriptionStr);

    return descriptionStr;
}


//---------------------------------------------------------------------------
// Get the detector serial number
/*!
@return detector serial number string
 */
//---------------------------------------------------------------------------
std::string EigerAdapter::getDetectorNumber()
{
    std::string numberStr;
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("detector_number")));
    value->get(numberStr);

    return numberStr;
}


//-----------------------------------------------------------------------------
/// return the detector pixel size in meter
/*!
@return EigerSize object containing pixel dimensions
 */
//-----------------------------------------------------------------------------
EigerSize EigerAdapter::getPixelSize(void)
{
    int sizex, sizey;

    std::shared_ptr<ResourceValue> valueXsize (dynamic_cast<ResourceValue*> (m_factory->getResource("x_pixel_size")));
    valueXsize->get(sizex);

    std::shared_ptr<ResourceValue> valueYsize (dynamic_cast<ResourceValue*> (m_factory->getResource("y_pixel_size")));
    valueYsize->get(sizey);

    return EigerSize(sizex, sizey);
}


//-----------------------------------------------------------------------------
/// return the detector dimensions in pixel
/*!
@return EigerSize object containing detector width and heigth
 */
//-----------------------------------------------------------------------------
EigerSize EigerAdapter::getDetectorSize(void)
{
    int width, height;

    std::shared_ptr<ResourceValue> valueWidth (dynamic_cast<ResourceValue*> (m_factory->getResource("detector_witdh")));
    valueWidth->get(width);

    std::shared_ptr<ResourceValue> valueHeight (dynamic_cast<ResourceValue*> (m_factory->getResource("detector_height")));
    valueHeight->get(height);

    return EigerSize(width, height);
}


//-----------------------------------------------------------------------------
/// Set the number of images per file
//-----------------------------------------------------------------------------
void EigerAdapter::setNbImagesPerFile(const int imagesPerFile)
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("nimages_per_file")));
    value->set(imagesPerFile);
}


//-----------------------------------------------------------------------------
/// Set the number of images to acquire
//-----------------------------------------------------------------------------
void EigerAdapter::setNbImages(const int nimages)
{
    std::shared_ptr<ResourceValue> value (dynamic_cast<ResourceValue*> (m_factory->getResource("nimages")));
    value->set(nimages);

    // Update images per file accordingly
    setNbImagesPerFile(nimages);
}


//---------------------------------------------------------------------------
// Request the API version installed on the Eiger server
/*!
@return string version (ex: "0.8.1")
 */
//---------------------------------------------------------------------------
std::string EigerAdapter::GetAPIVersion()
{
  std::string url = std::string(HTTP_ROOT) +
                    m_ip_addr + 
                    std::string("/") + 
                    STR_SUBSYSTEM_DETECTOR + 
                    std::string("/") + 
                    std::string(STR_EIGER_API) + 
                    std::string("/") + 
                    STR_EIGER_VERSION + 
                    std::string("/");  

  RESTfulClient client;
  std::string value = client.get_parameter<std::string>(url);
  return value;
}


//---------------------------------------------------------------------------
// Get the status of the LZ4 compression feature
/*!
@return true:enabled, false:disabled
 */
//---------------------------------------------------------------------------
bool EigerAdapter::getCompression(void)
{
    std::shared_ptr<ResourceValue> compression (dynamic_cast<ResourceValue*> (m_factory->getResource("compression")));
    bool bCompression;
    compression->get(bCompression);

    return bCompression;
}


//---------------------------------------------------------------------------
// Set the status of the LZ4 compression feature
//---------------------------------------------------------------------------
void EigerAdapter::setCompression(const bool enabled)  ///< [in] true:enabled, false:disabled
{
    std::shared_ptr<ResourceValue> compression (dynamic_cast<ResourceValue*> (m_factory->getResource("compression")));
    compression->set(enabled);
}

//---------------------------------------------------------------------------
// Search the index of a string in a vector
/*!
@return index of the string or -1 if not found
 */
//---------------------------------------------------------------------------
int EigerAdapter::GetIndex(const std::vector<std::string>& vect,      ///< [in] vector to search into
                           const std::string& str)                    ///< [in] string to search for
{
    int index = -1;
    for (unsigned int i = 0; i < vect.size(); i++)
    {
        if (str == vect.at(i))
        {
            index = i;
            break;
        }
    }

    return index;
}


}
