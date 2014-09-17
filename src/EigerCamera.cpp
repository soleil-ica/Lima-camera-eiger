///###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2014
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//############################################################################

#include <sstream>
#include <iostream>
#include <string>
#include <math.h>
#include <algorithm>
#include "EigerCamera.h"
#include <Timestamp.h>

using namespace lima;
using namespace lima::Eiger;
using namespace std;


//-----------------------------------------------------------------------------
/// utility thread
//-----------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//! Camera::CameraThread::CameraThread()
//---------------------------------------------------------------------------------------
Camera::CameraThread::CameraThread(Camera& cam)
: m_cam(&cam)
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "CameraThread::CameraThread - BEGIN";
    m_force_stop = false;
    DEB_TRACE() << "CameraThread::CameraThread - END";
}

//---------------------------------------------------------------------------------------
//! Camera::CameraThread::start()
//---------------------------------------------------------------------------------------
void Camera::CameraThread::start()
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "CameraThread::start - BEGIN";
    CmdThread::start();
    waitStatus(Ready);
    DEB_TRACE() << "CameraThread::start - END";
}

//---------------------------------------------------------------------------------------
//! Camera::CameraThread::init()
//---------------------------------------------------------------------------------------
void Camera::CameraThread::init()
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "CameraThread::init - BEGIN";
    setStatus(Ready);
    DEB_TRACE() << "CameraThread::init - END";
}

//---------------------------------------------------------------------------------------
//! Camera::CameraThread::execCmd()
//---------------------------------------------------------------------------------------
void Camera::CameraThread::execCmd(int cmd)
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "CameraThread::execCmd - BEGIN";
    int status = getStatus();
    switch (cmd)
    {
      case StartAcq:
        if (status != Ready)
        throw LIMA_HW_EXC(InvalidValue, "Not Ready to StartAcq");
        execStartAcq();
      break;
    }
    DEB_TRACE() << "CameraThread::execCmd - END";
}


//-----------------------------------------------------------------------------
///  Waits for a specific device state
//-----------------------------------------------------------------------------
void Camera::CameraThread::WaitForState(eigerapi::ENUM_STATE eTargetState) ///< [in] state to wait for
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "CameraThread::WaitForState - BEGIN";

    int iterCount = 0; // iteration counter
    eigerapi::ENUM_STATE eState;
    do
    {      
        try
        {
            eState = m_cam->m_pEigerAPI->getState(eigerapi::ESUBSYSTEM_FILEWRITER);
        }
        catch (eigerapi::EigerException &e)
        {
            HANDLE_EIGERERROR(e.what());
        }

        iterCount++;

        // Check for faulty states
        switch (eState)
        {
            case eigerapi::ESTATE_DISABLED:
            case eigerapi::ESTATE_ERROR:
            {
              DEB_ERROR() << "Faulty state reached during WaitForState.";
              THROW_HW_ERROR(Error) << "Faulty state reached during WaitForState.";
            }
            break;
        }

        // Check for operation timeout
        if ((eTargetState!=eState) && (iterCount*C_DETECTOR_POLL_TIME >= C_DETECTOR_MAX_TIME) )
        {
            DEB_ERROR() << "Timeout reached during WaitForState.";
            THROW_HW_ERROR(Error) << "Timeout reached during WaitForState.";
        }

        // If target state still not reached, wait before requesting the state again
        if (eTargetState!=eState)
        {
            lima::Sleep(C_DETECTOR_POLL_TIME);
        }
    }
    while (eTargetState!=eState);

    DEB_TRACE() << "CameraThread::WaitForState - END";
}


//---------------------------------------------------------------------------------------
//! Thread capture function for default capture mode
//---------------------------------------------------------------------------------------
void Camera::CameraThread::execStartAcq()
{
    DEB_MEMBER_FUNCT();

    DEB_TRACE() << "CameraThread::execStartAcq - BEGIN";
    setStatus(Exposure);

    StdBufferCbMgr& buffer_mgr = m_cam->m_buffer_ctrl_obj.getBuffer();

    FrameDim frame_dim = buffer_mgr.getFrameDim();
    Size frame_size    = frame_dim.getSize();
    int height         = frame_size.getHeight();
    int width          = frame_size.getWidth();

    DEB_TRACE() << "Run";

    // Wait for filewriter status to be ready
    WaitForState(eigerapi::ESTATE_READY);

    // Download and open the captured file for reading
    try
    {
        m_cam->m_pEigerAPI->downloadAcquiredFile(m_cam->m_targetFilename);
    }
    catch (eigerapi::EigerException &e)
    {
        HANDLE_EIGERERROR(e.what());
    }

  	// Begin to transfer the images to Lima
    buffer_mgr.setStartTimestamp(Timestamp::now());

  	// Acquisition loop
    bool bAcquisitionEnd = false;
    while ( (!bAcquisitionEnd) && ((0==m_cam->m_nb_frames) || (m_cam->m_image_number < m_cam->m_nb_frames)) )
    {   
        // Check if acquisition was stopped by device command & abort the current acquisition
        if (m_force_stop)
        {
            m_force_stop = false;
            break;
        }

        // Get the next image from the data file
        void* src = m_cam->m_pEigerAPI->getFrame();

        if (NULL!=src)
        {
            // Get a new frame buffer
            DEB_TRACE() << "getFrameBufferPtr(" << m_cam->m_image_number << ")";
            void *dst = buffer_mgr.getFrameBufferPtr(m_cam->m_image_number);      

            HwFrameInfoType frame_info;
            frame_info.acq_frame_nb = m_cam->m_image_number;		
            memcpy(dst, src, width * height );

            if (buffer_mgr.newFrameReady(frame_info))
            {
                ++m_cam->m_image_number;      
            }
            else
            {
                HANDLE_EIGERERROR("newFrameReady failure.");
            }
        }
        else
        {
            bAcquisitionEnd = true; // no more images are available in the data file
        }

    } /* end of acquisition loop */

    // Delete the acquired file
    try
    {
        m_cam->m_pEigerAPI->deleteAcquiredFile();
    }
    catch (eigerapi::EigerException &e)
    {
        HANDLE_EIGERERROR(e.what());
    }

    setStatus(Ready);

    DEB_TRACE() << "CameraThread::execStartAcq - END";
}


//-----------------------------------------------------------------------------
///  Ctor
//-----------------------------------------------------------------------------
Camera::Camera(const std::string& detector_ip,	///< [in] Ip address of the detector server
			         const std::string& target_path)	///< [in] temporary path where to store downloaded files
              : m_thread(*this),
    m_status(Ready),
    m_image_number(0),
    m_depth(16),
    m_latency_time(0.),
    m_exp_time(1.),
    m_targetFilename(target_path),
    m_pEigerAPI(NULL)
{
    DEB_CONSTRUCTOR();

    m_targetFilename += C_DOWNLOADED_FILENAME;

    // Init EigerAPI
    try
    {
      m_pEigerAPI = new eigerapi::EigerAdapter(detector_ip);

      // --- Initialise deeper parameters of the controller
      initialiseController();

      Size sizeMax;
      getDetectorImageSize(sizeMax);

      // Store max image size
      m_maxImageWidth  = sizeMax.getWidth();
      m_maxImageHeight = sizeMax.getHeight();      
    }
    catch (eigerapi::EigerException &e)
    {
        HANDLE_EIGERERROR(e.what());
    }

    // Display max image size
    DEB_TRACE() << "Detector max width: " << m_maxImageWidth;
    DEB_TRACE() << "Detector max height:" << m_maxImageHeight;

    // --- Set detector for software single image mode    
    setTrigMode(IntTrig);

    m_nb_frames = 1;

    // --- finally start the acq thread
    m_thread.start();
}


//-----------------------------------------------------------------------------
///  Dtor
//-----------------------------------------------------------------------------
Camera::~Camera()
{
    DEB_DESTRUCTOR();
    stopAcq();              
  
    try
    {   
        delete m_pEigerAPI;
    }
    catch (eigerapi::EigerException &e)
    {
        HANDLE_EIGERERROR(e.what());
    }
}


//-----------------------------------------------------------------------------
/// Set detector for single image acquisition
//-----------------------------------------------------------------------------
void Camera::prepareAcq()
{
	DEB_MEMBER_FUNCT();

    // initialize ?

    // send the arm command
    EIGER_EXEC(m_pEigerAPI->arm());
}


//-----------------------------------------------------------------------------
///  start the acquisition
//-----------------------------------------------------------------------------
void Camera::startAcq()
{
    DEB_MEMBER_FUNCT();

    // send the trigger command   
    EIGER_EXEC(m_pEigerAPI->trigger());

    // init force stop flag before starting acq thread
    m_thread.m_force_stop = false;

    // Start the thread
    m_image_number = 0;   
    m_thread.sendCmd(CameraThread::StartAcq);

    // Wait running state of acquisition thread
    m_thread.waitNotStatus(CameraThread::Ready);
}


//-----------------------------------------------------------------------------
/// stop the acquisition
//-----------------------------------------------------------------------------
void Camera::stopAcq()
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_pEigerAPI->disarm()); // send the disarm command

    m_thread.m_force_stop = true;

    m_thread.sendCmd(CameraThread::StopAcq);

    // Wait for thread to finish
    m_thread.waitStatus(CameraThread::Ready);
}


//-----------------------------------------------------------------------------
/// return the detector Max image size 
//-----------------------------------------------------------------------------
void Camera::getDetectorMaxImageSize(Size& size) ///< [out] image dimensions
{
	DEB_MEMBER_FUNCT();
	size = Size(m_maxImageWidth, m_maxImageHeight);
}


//-----------------------------------------------------------------------------
/// return the detector image size 
//-----------------------------------------------------------------------------
void Camera::getDetectorImageSize(Size& size) ///< [out] image dimensions
{
  DEB_MEMBER_FUNCT();

  int xmax = C_EIGERAPI_EIGER1M_WIDTH;
  int ymax = C_EIGERAPI_EIGER1M_HEIGHT;    
  size = Size(xmax, ymax);
}


//-----------------------------------------------------------------------------
/// Get the image type
//-----------------------------------------------------------------------------
void Camera::getImageType(ImageType& type) ///< [out] image type
{
 DEB_MEMBER_FUNCT();

 type = Bpp16;
}


//-----------------------------------------------------------------------------
//! Camera::setImageType()
//-----------------------------------------------------------------------------
void Camera::setImageType(ImageType type) ///< [in] image type
{
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::setImageType - " << DEB_VAR1(type);
}


//-----------------------------------------------------------------------------
/// return the detector type
//-----------------------------------------------------------------------------
void Camera::getDetectorType(string& type) ///< [out] detector type
{
  DEB_MEMBER_FUNCT();

  type = m_detector_type;
}


//-----------------------------------------------------------------------------
/// return the detector model
//-----------------------------------------------------------------------------
void Camera::getDetectorModel(string& model) ///< [out] detector model
{
  DEB_MEMBER_FUNCT();

  model = m_detector_model;
}


//-----------------------------------------------------------------------------
/// return the internal buffer manager
/*!
@ return buffer control object
*/
//-----------------------------------------------------------------------------
HwBufferCtrlObj* Camera::getBufferCtrlObj()
{
  DEB_MEMBER_FUNCT();
  return &m_buffer_ctrl_obj;
}


//-----------------------------------------------------------------------------
/// Checks trigger mode
/*!
@return true if the given trigger mode is supported
*/
//-----------------------------------------------------------------------------
bool Camera::checkTrigMode(TrigMode trig_mode) ///< [in] trigger mode to check
{
  DEB_MEMBER_FUNCT();
  DEB_PARAM() << DEB_VAR1(trig_mode);

  bool valid_mode = false;

  if (eigerapi::ETRIGMODE_UNKNOWN != getTriggerMode(trig_mode))
  {
    valid_mode = true;
  }

  return valid_mode;
}


//-----------------------------------------------------------------------------
/// Set the new trigger mode
//-----------------------------------------------------------------------------
void Camera::setTrigMode(TrigMode mode) ///< [in] lima trigger mode to set
{
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(mode);

    // Get the EIGERAPI mode associated to the given LiMA TrigMode
    try
    {
        eigerapi::ENUM_TRIGGERMODE eEigertrigMode = getTriggerMode(mode);
        if (eigerapi::ETRIGMODE_UNKNOWN != eEigertrigMode)
        {
            // set trigger mode using EigerAPI
            m_pEigerAPI->setTriggerMode(eEigertrigMode);
            m_trig_mode = mode;
        }
    }
    catch (eigerapi::EigerException &e)
    {
        HANDLE_EIGERERROR(e.what());
    }
}


//-----------------------------------------------------------------------------
/// Get the current trigger mode
//-----------------------------------------------------------------------------
void Camera::getTrigMode(TrigMode& mode) ///< [out] current trigger mode
{
  DEB_MEMBER_FUNCT();
  mode = m_trig_mode;

  DEB_RETURN() << DEB_VAR1(mode);
}


//-----------------------------------------------------------------------------
/// Set the new exposure time
//-----------------------------------------------------------------------------
void Camera::setExpTime(double exp_time) ///< [in] exposure time to set
{
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(exp_time);

    // set exposure time using EigerAPI
    EIGER_EXEC(m_pEigerAPI->setExposureTime(exp_time));

    m_exp_time = exp_time;
}


//-----------------------------------------------------------------------------
/// Get the current exposure time
//-----------------------------------------------------------------------------
void Camera::getExpTime(double& exp_time) ///< [out] current exposure time
{
 DEB_MEMBER_FUNCT();

 exp_time = m_exp_time;

 DEB_RETURN() << DEB_VAR1(exp_time);
}


//-----------------------------------------------------------------------------
/// Set the new latency time between images
//-----------------------------------------------------------------------------
void Camera::setLatTime(double lat_time) ///< [in] latency time
{
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(lat_time);

    try
    {
        if (lat_time >= 0.0)
        {
          m_pEigerAPI->setLatencyTime(lat_time);
          m_latency_time = lat_time;
        }
    }
    catch (eigerapi::EigerException &e)
    {
        HANDLE_EIGERERROR(e.what());
    }
}


//-----------------------------------------------------------------------------
/// Get the current latency time
//-----------------------------------------------------------------------------
void Camera::getLatTime(double& lat_time) ///< [out] current latency time
{
  DEB_MEMBER_FUNCT();
  
  lat_time = m_latency_time;

  DEB_RETURN() << DEB_VAR1(lat_time);
}


//-----------------------------------------------------------------------------
/// Get the exposure time range
//-----------------------------------------------------------------------------
void Camera::getExposureTimeRange(double& min_expo,	///< [out] minimum exposure time
                                  double& max_expo)   ///< [out] maximum exposure time
const
{
  DEB_MEMBER_FUNCT();

  long	capflags;
  double	step, defaultvalue;
   // get exposure time range using EigerAPI
  min_expo = 0.0;
  max_expo = 10.0;
	/*if(  )
	{
        HANDLE_EIGERERROR("Failed to get exposure time");
	}
   */

  DEB_RETURN() << DEB_VAR2(min_expo, max_expo);
}


//-----------------------------------------------------------------------------
///  Get the latency time range
//-----------------------------------------------------------------------------
void Camera::getLatTimeRange(double& min_lat, ///< [out] minimum latency
                             double& max_lat) ///< [out] maximum latency
const
{
  DEB_MEMBER_FUNCT();

    // --- no info on min latency
  min_lat = 0.;

    // --- do not know how to get the max_lat, fix it as the max exposure time
  max_lat = m_exp_time_max;

  DEB_RETURN() << DEB_VAR2(min_lat, max_lat);
}


//-----------------------------------------------------------------------------
/// Set the number of frames to be taken
//-----------------------------------------------------------------------------
void Camera::setNbFrames(int nb_frames) ///< [in] number of frames to take
{
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(nb_frames);

    if (0==nb_frames)
    {
        HANDLE_EIGERERROR("video mode is not supported.");
    }

    m_nb_frames = nb_frames;
}


//-----------------------------------------------------------------------------
/// Get the number of frames to be taken
//-----------------------------------------------------------------------------
void Camera::getNbFrames(int& nb_frames) ///< [out] current number of frames to take
{
  DEB_MEMBER_FUNCT();
  nb_frames = m_nb_frames;
  DEB_RETURN() << DEB_VAR1(nb_frames);
}


//-----------------------------------------------------------------------------
/// Get the current acquired frames
//-----------------------------------------------------------------------------
void Camera::getNbHwAcquiredFrames(int &nb_acq_frames) ///< [out] number of acquired files
{ 
  DEB_MEMBER_FUNCT();    
  nb_acq_frames = m_image_number;
}


//-----------------------------------------------------------------------------
/// Get the camera status
//-----------------------------------------------------------------------------
Camera::Status Camera::getStatus() ///< [out] current camera status
{
    DEB_MEMBER_FUNCT();

    int thread_status = m_thread.getStatus();

    DEB_RETURN() << DEB_VAR1(thread_status);

    switch (thread_status)
    {
        case CameraThread::Ready:
          return Camera::Ready;

        case CameraThread::Exposure:
          return Camera::Exposure;

        case CameraThread::Readout:
          return Camera::Readout;

        case CameraThread::Latency:
          return Camera::Latency;

        default:
          throw LIMA_HW_EXC(Error, "Invalid thread status");
    }
}


//-----------------------------------------------------------------------------
/// Check if a binning value is supported
/*
@return true if the given binning value exists
*/
//-----------------------------------------------------------------------------
bool Camera::isBinningSupported(const int binValue)	///< [in] binning value to check for
{
	DEB_MEMBER_FUNCT();

	return false; // No binning available on Eiger
}


//-----------------------------------------------------------------------------
/// Tells if binning is available
/*!
@return always false, hw binning mode is not supported
*/
//-----------------------------------------------------------------------------
bool Camera::isBinningAvailable()
{
  DEB_MEMBER_FUNCT();
  return false;
}


//-----------------------------------------------------------------------------
/// return the detector pixel size in meter
//-----------------------------------------------------------------------------
void Camera::getPixelSize(double& sizex,	///< [out] horizontal pixel size
                          double& sizey)	///< [out] vertical   pixel size
{
  DEB_MEMBER_FUNCT();

  sizex = m_x_pixelsize;
  sizey = m_y_pixelsize;
  DEB_RETURN() << DEB_VAR2(sizex, sizey); 
}


//-----------------------------------------------------------------------------
/// reset the camera, no hw reset available on Eiger camera
//-----------------------------------------------------------------------------
/*
void Camera::reset()
{
    DEB_MEMBER_FUNCT();
    return;
}
*/

//-----------------------------------------------------------------------------
///    initialise controller with speeds and preamp gain
//-----------------------------------------------------------------------------
void Camera::initialiseController()
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "initialiseController()";

    // Fills the map of available trigger modes
    m_map_trig_modes[IntTrig] 		  = eigerapi::ETRIGMODE_EXPO;
    m_map_trig_modes[ExtTrigSingle] = eigerapi::ETRIGMODE_EXTT;
    m_map_trig_modes[ExtTrigMult]	  = eigerapi::ETRIGMODE_EXTM;
    m_map_trig_modes[ExtGate]       = eigerapi::ETRIGMODE_EXTTE;

    // Send an "initialize" command
    DEB_TRACE() << "m_pEigerAPI->initialize()";
    m_pEigerAPI->initialize();

    // Retrieve pixel size
    DEB_TRACE() << "m_pEigerAPI->getPixelSize()";
    eigerapi::EigerSize eSz = m_pEigerAPI->getPixelSize();
    m_x_pixelsize = eSz.getX();
    m_y_pixelsize = eSz.getY();

    // Detector model
    DEB_TRACE() << "m_pEigerAPI->getDescription()";
    m_detector_model = m_pEigerAPI->getDescription();   

    // Detector number
    DEB_TRACE() << "m_pEigerAPI->getDetectorNumber";
    m_detector_type = m_pEigerAPI->getDetectorNumber();

    // Retrieve exposure time
    DEB_TRACE() << "m_pEigerAPI->getExposureTime()";
    m_exp_time = m_pEigerAPI->getExposureTime();

    //double min_expo, max_expo;
    // Exposure time
    // DEB_TRACE() << "Min exposure time: " << min_expo;
    // DEB_TRACE() << "Max exposure time: " << max_expo;

    m_exp_time_max = 10.0; // TODO: implement the getminmax in RestFul client/ ResourceValue
 }


//-----------------------------------------------------------------------------
/// Get the eiger api trigger mode value associated to the given Lima TrigMode 
/*!
@return eiger api trigger mode or ETRIGMODE_UNKNOWN if no associated value found
*/
//-----------------------------------------------------------------------------
eigerapi::ENUM_TRIGGERMODE Camera::getTriggerMode(const TrigMode trig_mode) ///< [in] lima trigger mode value
{
    map<TrigMode, eigerapi::ENUM_TRIGGERMODE>::const_iterator iterFind = m_map_trig_modes.find(trig_mode);
    if (m_map_trig_modes.end()!=iterFind)
    {
      return iterFind->second;
    }
    else
    {
      return eigerapi::ETRIGMODE_UNKNOWN;
    }
}


//-----------------------------------------------------------------------------
/// Returns the temperature of the detector
/*!
@return temperature value
*/
//-----------------------------------------------------------------------------
double Camera::getTemperature()
{
  DEB_MEMBER_FUNCT();

  EIGER_EXEC(return m_pEigerAPI->getTemperature());
}


//-----------------------------------------------------------------------------
/// Returns the humidity of the detector
/*!
@return humidity value
*/
//-----------------------------------------------------------------------------
double Camera::getHumidity()
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_pEigerAPI->getHumidity());
}


//-----------------------------------------------------------------------------
///  Count rate correction setter
//-----------------------------------------------------------------------------
void Camera::setCountrateCorrection(const bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_pEigerAPI->setCountrateCorrection(value));
}


//-----------------------------------------------------------------------------
///  Count rate correction getter
//-----------------------------------------------------------------------------
void Camera::getCountrateCorrection(bool& value)  ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();  

    EIGER_EXEC(value = m_pEigerAPI->getCountrateCorrection());
}


//-----------------------------------------------------------------------------
///  FlatfieldCorrection setter
//-----------------------------------------------------------------------------
void Camera::setFlatfieldCorrection(const bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_pEigerAPI->setFlatfieldCorrection(value));
}


//-----------------------------------------------------------------------------
///  FlatfieldCorrection getter
//-----------------------------------------------------------------------------
void Camera::getFlatfieldCorrection(bool& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_pEigerAPI->getFlatfieldCorrection());
}


//-----------------------------------------------------------------------------
///  PixelMask setter
//-----------------------------------------------------------------------------
void Camera::setPixelMask(const bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_pEigerAPI->setPixelMask(value));
}


//-----------------------------------------------------------------------------
///  PixelMask getter
//-----------------------------------------------------------------------------
void Camera::getPixelMask(bool& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_pEigerAPI->getPixelMask());
}

//-----------------------------------------------------------------------------
/// EfficiencyCorrection setter
//-----------------------------------------------------------------------------
void Camera::setEfficiencyCorrection(const bool enabled) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_pEigerAPI->setEfficiencyCorrection(enabled));
}


//-----------------------------------------------------------------------------
/// EfficiencyCorrection getter
//-----------------------------------------------------------------------------
void Camera::getEfficiencyCorrection(bool& value)  ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_pEigerAPI->getEfficiencyCorrection());
}


//-----------------------------------------------------------------------------
///  ThresholdEnergy setter
//-----------------------------------------------------------------------------
void Camera::setThresholdEnergy(const double value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_pEigerAPI->setThresholdEnergy(value));
}


//-----------------------------------------------------------------------------
///  ThresholdEnergy getter
//-----------------------------------------------------------------------------
void Camera::getThresholdEnergy(double& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_pEigerAPI->getThresholdEnergy());
}


//-----------------------------------------------------------------------------
///  VirtualPixelCorrection setter
//-----------------------------------------------------------------------------
void Camera::setVirtualPixelCorrection(const bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_pEigerAPI->setVirtualPixelCorrection(value));
}


//-----------------------------------------------------------------------------
///  VirtualPixelCorrection getter
//-----------------------------------------------------------------------------
void Camera::getVirtualPixelCorrection(bool& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_pEigerAPI->getVirtualPixelCorrection());
}


//-----------------------------------------------------------------------------
///  PhotonEnergy setter
//-----------------------------------------------------------------------------
void Camera::setPhotonEnergy(const double value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_pEigerAPI->setPhotonEnergy(value));
}


//-----------------------------------------------------------------------------
///  PhotonEnergy getter
//-----------------------------------------------------------------------------
void Camera::getPhotonEnergy(double& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_pEigerAPI->getPhotonEnergy());
}

