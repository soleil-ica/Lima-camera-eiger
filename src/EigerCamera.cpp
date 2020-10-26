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
#include <eigerapi/Requests.h>
#include "lima/Timestamp.h"

using namespace lima;
using namespace lima::Eiger;
using namespace std;
using namespace eigerapi;


#define HANDLE_EIGERERROR(__errMsg__) {		\
THROW_HW_ERROR(Error) << __errMsg__;		\
}

#define EIGER_SYNC_CMD_TIMEOUT(CommandType,timeout)			\
  {									\
    std::shared_ptr<Requests::Command> cmd =				\
      m_requests->get_command(CommandType);				\
    try									\
      {									\
	cmd->wait(timeout);						\
      }									\
    catch(const eigerapi::EigerException &e)				\
      {									\
	m_requests->cancel(cmd);					\
	HANDLE_EIGERERROR(e.what());					\
      }									\
  }

#define EIGER_SYNC_CMD(CommandType)		\
  EIGER_SYNC_CMD_TIMEOUT(CommandType,CurlLoop::FutureRequest::TIMEOUT)
  
#define EIGER_SYNC_SET_PARAM(ParamType,value)				\
  {									\
    std::shared_ptr<Requests::Param> req =				\
      m_requests->set_param(ParamType,value);				\
    try									\
      {									\
	req->wait();							\
      }									\
    catch(const eigerapi::EigerException &e)				\
      {									\
	HANDLE_EIGERERROR(e.what());					\
      }									\
  }

#define EIGER_SYNC_GET_PARAM(ParamType,value)				\
  {									\
    std::shared_ptr<Requests::Param> req =				\
      m_requests->get_param(ParamType,value);				\
    try									\
      {									\
	req->wait();							\
      }									\
    catch(const eigerapi::EigerException &e)				\
      {									\
	m_requests->cancel(req);					\
	HANDLE_EIGERERROR(e.what());					\
      }									\
  }

/*----------------------------------------------------------------------------
			    Callback class
 ----------------------------------------------------------------------------*/
class Camera::AcqCallback : public CurlLoop::FutureRequest::Callback
{
public:
  AcqCallback(Camera& cam) : m_cam(cam) {}

  void status_changed(CurlLoop::FutureRequest::Status status)
  {
    m_cam._acquisition_finished(status == CurlLoop::FutureRequest::OK);
  }
private:
  Camera& m_cam;
};

class Camera::InitCallback : public CurlLoop::FutureRequest::Callback
{	
  DEB_CLASS_NAMESPC(DebModCamera, "Camera", "Eiger::InitCallback");
public:
  InitCallback(Camera& cam) : m_cam(cam) {}

  void status_changed(CurlLoop::FutureRequest::Status status)
  {
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(status);

    AutoMutex lock(m_cam.m_cond.mutex());
    if(status == CurlLoop::FutureRequest::OK)
      m_cam.m_initilize_state= Camera::IDLE;
    else
      m_cam.m_initilize_state = Camera::ERROR;

    DEB_ALWAYS() << "Initialize finished";
  }
private:
  Camera& m_cam;
};

//-----------------------------------------------------------------------------
///  Ctor
//-----------------------------------------------------------------------------
Camera::Camera(const std::string& detector_ip)	///< [in] Ip address of the detector server
  : 		m_image_number(0),
                m_latency_time(0.),
                m_detectorImageType(Bpp16),
		m_initilize_state(IDLE),
		m_trigger_state(IDLE),
		m_serie_id(0),
                m_requests(new Requests(detector_ip)),
                m_exp_time(1.),
		m_detector_ip(detector_ip),
		m_timestamp_type("RELATIVE")
{
    DEB_CONSTRUCTOR();
    DEB_PARAM() << DEB_VAR1(detector_ip);
    // Init EigerAPI
    try
      {
	initialiseController();
      }
    catch(Exception& e)
      {
	DEB_ALWAYS() << "Could not get configuration parameters, try to initialize";
	EIGER_SYNC_CMD_TIMEOUT(Requests::INITIALIZE,5*60);
	initialiseController();
      } 

    // Display max image size
    DEB_TRACE() << "Detector max width: " << m_maxImageWidth;
    DEB_TRACE() << "Detector max height:" << m_maxImageHeight;

    // --- Set detector for software single image mode    
    setTrigMode(IntTrig);

    m_nb_frames = 1;

}


//-----------------------------------------------------------------------------
///  Dtor
//-----------------------------------------------------------------------------
Camera::~Camera()
{
    DEB_DESTRUCTOR();
    delete m_requests;
}


//----------------------------------------------------------------------------
// initialize detector
//----------------------------------------------------------------------------
void Camera::initialize()
{
  DEB_MEMBER_FUNCT();
  // Finally initialize the detector
  AutoMutex lock(m_cond.mutex());
  m_initilize_state = RUNNING;
  std::shared_ptr<Requests::Command> async_initialise =
    m_requests->get_command(Requests::INITIALIZE);
  lock.unlock();

  std::shared_ptr<CurlLoop::FutureRequest::Callback> cbk(new InitCallback(*this));
  async_initialise->register_callback(cbk);
}
//-----------------------------------------------------------------------------
/// Set detector for single image acquisition
//-----------------------------------------------------------------------------
void Camera::prepareAcq()
{
  DEB_MEMBER_FUNCT();
  AutoMutex aLock(m_cond.mutex());
  if(m_trigger_state != IDLE)
    EIGER_SYNC_CMD(Requests::DISARM);
  
  int nb_frames;
  unsigned nb_trigger;
  switch(m_trig_mode)
    {
    case IntTrig:
    case ExtTrigSingle:
      nb_frames = m_nb_frames,nb_trigger = 1;break;
    case IntTrigMult:
    case ExtTrigMult:
    case ExtGate:
      nb_frames = 1,nb_trigger = m_nb_frames;break;
    default:
      THROW_HW_ERROR(Error) << "Very weird can't be in this case";
    }
  double frame_time = m_exp_time + m_latency_time;
  if(frame_time < m_min_frame_time)
    {    
      if(m_latency_time <= m_readout_time)
	frame_time = m_min_frame_time;
      else
	THROW_HW_ERROR(Error) << "This detector can't go at this frame rate (" << 1 / frame_time
			      << ") is limited to (" << 1 / m_min_frame_time << ")";
    }
  DEB_PARAM() << DEB_VAR1(frame_time);
  std::shared_ptr<Requests::Param> frame_time_req=
    m_requests->set_param(Requests::FRAME_TIME,frame_time);
  std::shared_ptr<Requests::Param> nimages_req =
    m_requests->set_param(Requests::NIMAGES,nb_frames);
  std::shared_ptr<Requests::Param> ntrigger_req =
    m_requests->set_param(Requests::NTRIGGER,nb_trigger);

  try
    {
      frame_time_req->wait();
      nimages_req->wait();
      ntrigger_req->wait();
    }
  catch(const eigerapi::EigerException &e)
    {
      HANDLE_EIGERERROR(e.what());
    }

  DEB_TRACE() << "Arm start";
  double timeout = 5 * 60.; // 5 min timeout
  std::shared_ptr<Requests::Command> arm_cmd =
    m_requests->get_command(Requests::ARM);
  try
    {
      arm_cmd->wait(timeout);
      DEB_TRACE() << "Arm end";
      m_serie_id = arm_cmd->get_serie_id();
    }
  catch(const eigerapi::EigerException &e)
    {
      m_requests->cancel(arm_cmd);
      HANDLE_EIGERERROR(e.what());
    }
  m_image_number = 0;
}


//-----------------------------------------------------------------------------
///  start the acquisition
//-----------------------------------------------------------------------------
void Camera::startAcq()
{
  DEB_MEMBER_FUNCT();
  AutoMutex lock(m_cond.mutex());


  if(m_trig_mode == IntTrig ||
     m_trig_mode == IntTrigMult)
    {
      std::shared_ptr<Requests::Command> trigger =
	m_requests->get_command(Requests::TRIGGER);
      m_trigger_state = RUNNING;
      lock.unlock();

      std::shared_ptr<CurlLoop::FutureRequest::Callback> cbk(new AcqCallback(*this));
      trigger->register_callback(cbk);
    }
  
}


//-----------------------------------------------------------------------------
/// stop the acquisition
//-----------------------------------------------------------------------------
void Camera::stopAcq()
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_CMD(Requests::ABORT);
}

//-----------------------------------------------------------------------------
/// update detector status(temperature/humidity)
//-----------------------------------------------------------------------------
void Camera::statusUpdate()
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_CMD(Requests::STATUS_UPDATE);
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

  std::shared_ptr<Requests::Param> width_request = 
    m_requests->get_param(Requests::DETECTOR_WITDH);
  std::shared_ptr<Requests::Param> height_request = 
    m_requests->get_param(Requests::DETECTOR_HEIGHT);

  Requests::Param::Value width = width_request->get();
  Requests::Param::Value height = height_request->get();
  size = Size(width.data.int_val, height.data.int_val);
}


//-----------------------------------------------------------------------------
/// Get the image type
//-----------------------------------------------------------------------------
void Camera::getImageType(ImageType& type) ///< [out] image type
{
    DEB_MEMBER_FUNCT();

    type = m_detectorImageType;    
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
/// Checks trigger mode
/*!
@return true if the given trigger mode is supported
*/
//-----------------------------------------------------------------------------
bool Camera::checkTrigMode(TrigMode trig_mode) ///< [in] trigger mode to check
{
  DEB_MEMBER_FUNCT();
  DEB_PARAM() << DEB_VAR1(trig_mode);
  switch(trig_mode)
    {
    case IntTrig:
    case IntTrigMult:
    case ExtTrigSingle:
    case ExtTrigMult:
    case ExtGate:
      return true;
    default:
      return false;
    }
}


//-----------------------------------------------------------------------------
/// Set the new trigger mode
//-----------------------------------------------------------------------------
void Camera::setTrigMode(TrigMode trig_mode) ///< [in] lima trigger mode to set
{
  DEB_MEMBER_FUNCT();
  DEB_PARAM() << DEB_VAR1(trig_mode);

  const char *trig_name;
  switch(trig_mode)
    {
    case IntTrig:
    case IntTrigMult:
      trig_name = "ints";break;
    case ExtTrigSingle:
    case ExtTrigMult:
      trig_name = "exts";break;
    case ExtGate:
      trig_name = "exte";break;
    default:
      THROW_HW_ERROR(NotSupported) << DEB_VAR1(trig_mode);
    }
  
  EIGER_SYNC_SET_PARAM(Requests::TRIGGER_MODE,trig_name);
  m_trig_mode = trig_mode;
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

  EIGER_SYNC_SET_PARAM(Requests::EXPOSURE,exp_time);
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

  m_latency_time = lat_time;
  setExpTime(m_exp_time);
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
  std::shared_ptr<Requests::Param> exp_time = 
    m_requests->get_param(Requests::EXPOSURE);
  try
    {
      exp_time->wait();
    }
  catch(const eigerapi::EigerException &e)
    {
      m_requests->cancel(exp_time);
      HANDLE_EIGERERROR(e.what());
    }

  Requests::Param::Value min_val = exp_time->get_min();
  Requests::Param::Value max_val = exp_time->get_max();
  
  min_expo = min_val.data.double_val;
  max_expo = max_val.data.double_val;
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
  min_lat = m_readout_time;
  double min_exp,max_exp;
  getExposureTimeRange(min_exp,max_exp);
  max_lat = max_exp;

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

  Camera::Status status;
  AutoMutex lock(m_cond.mutex());
  if(m_initilize_state == ERROR ||
     m_trigger_state == ERROR)
    status = Fault;
  else if(m_trigger_state == RUNNING)
    status = Exposure;
  else if(m_initilize_state == RUNNING)
    status = Initialising;
  else
    status = Ready;

  DEB_RETURN() << DEB_VAR1(status);
  return status;
}
//----------------------------------------------------------------------------
// Get camera hardware status
//----------------------------------------------------------------------------
std::string Camera::getCamStatus()
{
  DEB_MEMBER_FUNCT();
  std::string status;
  EIGER_SYNC_GET_PARAM(Requests::DETECTOR_STATUS,status);
  return status;
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
///    initialise controller
//-----------------------------------------------------------------------------
void Camera::initialiseController()
{
  DEB_MEMBER_FUNCT();
  DEB_TRACE() << "initialiseController()";

  std::list<std::shared_ptr<Requests::Param> > synchro_list;
  std::string trig_name;
  synchro_list.push_back(m_requests->get_param(Requests::TRIGGER_MODE,trig_name));
  
  synchro_list.push_back(m_requests->get_param(Requests::X_PIXEL_SIZE,m_x_pixelsize));
  synchro_list.push_back(m_requests->get_param(Requests::Y_PIXEL_SIZE,m_y_pixelsize));

  synchro_list.push_back(m_requests->get_param(Requests::DETECTOR_WITDH,m_maxImageWidth));
  synchro_list.push_back(m_requests->get_param(Requests::DETECTOR_HEIGHT,m_maxImageHeight));

  synchro_list.push_back(m_requests->get_param(Requests::DETECTOR_READOUT_TIME,m_readout_time));

  synchro_list.push_back(m_requests->get_param(Requests::DESCRIPTION,m_detector_model));
  synchro_list.push_back(m_requests->get_param(Requests::DETECTOR_NUMBER,m_detector_type));
  synchro_list.push_back(m_requests->get_param(Requests::EXPOSURE,m_exp_time));
  
  unsigned nb_trigger;
  synchro_list.push_back(m_requests->get_param(Requests::NTRIGGER,nb_trigger));

  std::shared_ptr<Requests::Param> frame_time_req = m_requests->get_param(Requests::FRAME_TIME);
  synchro_list.push_back(frame_time_req);

  bool auto_summation;
  synchro_list.push_back(m_requests->get_param(Requests::AUTO_SUMMATION,
					       auto_summation));
  
  synchro_list.push_back(m_requests->get_param(Requests::SOFTWARE_VERSION,m_software_version));

  //Synchro
  try
    {
      for(std::list<shared_ptr<Requests::Param> >::iterator i = synchro_list.begin();
	  i != synchro_list.end();++i)
	(*i)->wait();
    } 
  catch(const eigerapi::EigerException &e)
    {
      for(std::list<shared_ptr<Requests::Param> >::iterator i = synchro_list.begin();
	  i != synchro_list.end();++i)
	m_requests->cancel(*i);

        HANDLE_EIGERERROR(e.what());
    }

  m_detectorImageType = auto_summation ? Bpp32 : Bpp16;

  //Trigger mode
  if(trig_name == "ints")
    m_trig_mode = nb_trigger > 1 ? IntTrigMult : IntTrig;
  else if(trig_name == "exts")
    m_trig_mode = nb_trigger > 1 ? ExtTrigMult : ExtTrigSingle;
  else if(trig_name == "exte")
    m_trig_mode = ExtGate;
  else
    THROW_HW_ERROR(InvalidValue) << "Unexpected trigger mode: " << DEB_VAR1(trig_name);
  
  Requests::Param::Value min_frame_time = frame_time_req->get_min();
  m_min_frame_time = min_frame_time.data.double_val;
 }
/*----------------------------------------------------------------------------
	This method is called when the acquisition is finished
  ----------------------------------------------------------------------------*/
void Camera::_acquisition_finished(bool ok)
{
  DEB_MEMBER_FUNCT();
  DEB_PARAM() << DEB_VAR1(ok);
  
  std::string error_msg;

  //First we will disarm
  if(ok)
  {
	  DEB_TRACE()<<"Camera::_acquisition_finished() : DISARM";
    std::shared_ptr<Requests::Command> disarm = 
      m_requests->get_command(Requests::DISARM);
  }

  AutoMutex lock(m_cond.mutex());
  m_trigger_state = ok ? IDLE : ERROR;
  if(!error_msg.empty())
    DEB_ERROR() << error_msg;
}
//-----------------------------------------------------------------------------
/// Returns the temperature of the detector
/*!
@return temperature value
*/
//-----------------------------------------------------------------------------
void Camera::getTemperature(double &temp)
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::TEMP,temp);
}


//-----------------------------------------------------------------------------
/// Returns the humidity of the detector
/*!
@return humidity value
*/
//-----------------------------------------------------------------------------
void Camera::getHumidity(double &humidity)
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HUMIDITY,humidity);
}


//-----------------------------------------------------------------------------
///  Count rate correction setter
//-----------------------------------------------------------------------------
void Camera::setCountrateCorrection(bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::COUNTRATE_CORRECTION,value);
}


//-----------------------------------------------------------------------------
///  Count rate correction getter
//-----------------------------------------------------------------------------
void Camera::getCountrateCorrection(bool& value)  ///< [out] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::COUNTRATE_CORRECTION,value);
}


//-----------------------------------------------------------------------------
///  FlatfieldCorrection setter
//-----------------------------------------------------------------------------
void Camera::setFlatfieldCorrection(bool value) ///< [in] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_SET_PARAM(Requests::FLATFIELD_CORRECTION,value);
}


//-----------------------------------------------------------------------------
///  FlatfieldCorrection getter
//-----------------------------------------------------------------------------
void Camera::getFlatfieldCorrection(bool& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::FLATFIELD_CORRECTION,value);
}

//----------------------------------------------------------------------------
// Auto Summation setter
//----------------------------------------------------------------------------
void Camera::setAutoSummation(bool value)
{
  DEB_MEMBER_FUNCT();
  DEB_PARAM() << DEB_VAR1(value);
  EIGER_SYNC_SET_PARAM(Requests::AUTO_SUMMATION,value);

  Size image_size;
  getDetectorImageSize(image_size);
  m_detectorImageType = value ? Bpp32 : Bpp16;
  maxImageSizeChanged(image_size,m_detectorImageType);
}

//----------------------------------------------------------------------------
// Auto Summation getter
//----------------------------------------------------------------------------
void Camera::getAutoSummation(bool& value)
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::AUTO_SUMMATION,value);
  DEB_RETURN() << DEB_VAR1(value);
}
//-----------------------------------------------------------------------------
///  PixelMask setter
//-----------------------------------------------------------------------------
void Camera::setPixelMask(bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::PIXEL_MASK,value);
}


//-----------------------------------------------------------------------------
///  PixelMask getter
//-----------------------------------------------------------------------------
void Camera::getPixelMask(bool& value) ///< [out] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::PIXEL_MASK,value);
}

//-----------------------------------------------------------------------------
/// EfficiencyCorrection setter
//-----------------------------------------------------------------------------
void Camera::setEfficiencyCorrection(bool enabled) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::EFFICIENCY_CORRECTION,enabled);
}


//-----------------------------------------------------------------------------
/// EfficiencyCorrection getter
//-----------------------------------------------------------------------------
void Camera::getEfficiencyCorrection(bool& value)  ///< [out] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::EFFICIENCY_CORRECTION,value);
}


//-----------------------------------------------------------------------------
///  ThresholdEnergy setter
//-----------------------------------------------------------------------------
void Camera::setThresholdEnergy(double value) ///< [in] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_SET_PARAM(Requests::THRESHOLD_ENERGY,value);
}


//-----------------------------------------------------------------------------
///  ThresholdEnergy getter
//-----------------------------------------------------------------------------
void Camera::getThresholdEnergy(double& value) ///< [out] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::THRESHOLD_ENERGY,value);
}


//-----------------------------------------------------------------------------
///  VirtualPixelCorrection setter
//-----------------------------------------------------------------------------
void Camera::setVirtualPixelCorrection(bool value) ///< [in] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_SET_PARAM(Requests::VIRTUAL_PIXEL_CORRECTION,value);
}


//-----------------------------------------------------------------------------
///  VirtualPixelCorrection getter
//-----------------------------------------------------------------------------
void Camera::getVirtualPixelCorrection(bool& value) ///< [out] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::VIRTUAL_PIXEL_CORRECTION,value);
}


//-----------------------------------------------------------------------------
///  PhotonEnergy setter
//-----------------------------------------------------------------------------
void Camera::setPhotonEnergy(double value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::PHOTON_ENERGY,value);
}


//-----------------------------------------------------------------------------
///  PhotonEnergy getter
//-----------------------------------------------------------------------------
void Camera::getPhotonEnergy(double& value) ///< [out] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::PHOTON_ENERGY,value);
}

//-----------------------------------------------------------------------------
///  Wavelength setter
//-----------------------------------------------------------------------------
void Camera::setWavelength(double value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_WAVELENGTH,value);
}


//-----------------------------------------------------------------------------
///  Wavelength getter
//-----------------------------------------------------------------------------
void Camera::getWavelength(double& value) ///< [out] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_WAVELENGTH,value);
}


//-----------------------------------------------------------------------------
///  BeamCenterX setter
//-----------------------------------------------------------------------------
void Camera::setBeamCenterX(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_BEAM_CENTER_X,value);
}

//-----------------------------------------------------------------------------
///  BeamCenterX getter
//-----------------------------------------------------------------------------
void Camera::getBeamCenterX(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_BEAM_CENTER_X,value);
}

//-----------------------------------------------------------------------------
///  BeamCenterY setter
//-----------------------------------------------------------------------------
void Camera::setBeamCenterY(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_BEAM_CENTER_Y,value);
}

//-----------------------------------------------------------------------------
///  BeamCenterY getter
//-----------------------------------------------------------------------------
void Camera::getBeamCenterY(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_BEAM_CENTER_Y,value);
}

//-----------------------------------------------------------------------------
///  DetectorDistance setter
//-----------------------------------------------------------------------------
void Camera::setDetectorDistance(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_DETECTOR_DISTANCE,value);
}

//-----------------------------------------------------------------------------
///  DetectorDistance getter
//-----------------------------------------------------------------------------
void Camera::getDetectorDistance(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_DETECTOR_DISTANCE,value);
}

//-----------------------------------------------------------------------------
///  ChiIncrement setter
//-----------------------------------------------------------------------------
void Camera::setChiIncrement(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_CHI_INCREMENT,value);
}

//-----------------------------------------------------------------------------
///  ChiIncrement getter
//-----------------------------------------------------------------------------
void Camera::getChiIncrement(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_CHI_INCREMENT,value);
}

//-----------------------------------------------------------------------------
///  ChiStart setter
//-----------------------------------------------------------------------------
void Camera::setChiStart(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_CHI_START,value);
}

//-----------------------------------------------------------------------------
///  ChiStart getter
//-----------------------------------------------------------------------------
void Camera::getChiStart(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_CHI_START,value);
}


//-----------------------------------------------------------------------------
///  KappaIncrement setter
//-----------------------------------------------------------------------------
void Camera::setKappaIncrement(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_KAPPA_INCREMENT,value);
}

//-----------------------------------------------------------------------------
///  KappaIncrement getter
//-----------------------------------------------------------------------------
void Camera::getKappaIncrement(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_KAPPA_INCREMENT,value);
}

//-----------------------------------------------------------------------------
///  KappaStart setter
//-----------------------------------------------------------------------------
void Camera::setKappaStart(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_KAPPA_START,value);
}

//-----------------------------------------------------------------------------
///  KappaStart getter
//-----------------------------------------------------------------------------
void Camera::getKappaStart(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_KAPPA_START,value);
}


//-----------------------------------------------------------------------------
///  OmegaIncrement setter
//-----------------------------------------------------------------------------
void Camera::setOmegaIncrement(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_OMEGA_INCREMENT,value);
}

//-----------------------------------------------------------------------------
///  OmegaIncrement getter
//-----------------------------------------------------------------------------
void Camera::getOmegaIncrement(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_OMEGA_INCREMENT,value);
}

//-----------------------------------------------------------------------------
///  OmegaStart setter
//-----------------------------------------------------------------------------
void Camera::setOmegaStart(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_OMEGA_START,value);
}

//-----------------------------------------------------------------------------
///  OmegaStart getter
//-----------------------------------------------------------------------------
void Camera::getOmegaStart(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_OMEGA_START,value);
}

//-----------------------------------------------------------------------------
///  PhiIncrement setter
//-----------------------------------------------------------------------------
void Camera::setPhiIncrement(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_PHI_INCREMENT,value);
}

//-----------------------------------------------------------------------------
///  PhiIncrement getter
//-----------------------------------------------------------------------------
void Camera::getPhiIncrement(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_PHI_INCREMENT,value);
}

//-----------------------------------------------------------------------------
///  PhiStart setter
//-----------------------------------------------------------------------------
void Camera::setPhiStart(double value) ///< [in] 
{
    DEB_MEMBER_FUNCT();
    EIGER_SYNC_SET_PARAM(Requests::HEADER_PHI_START,value);
}

//-----------------------------------------------------------------------------
///  PhiStart getter
//-----------------------------------------------------------------------------
void Camera::getPhiStart(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::HEADER_PHI_START,value);
}

//-----------------------------------------------------------------------------
///  DataCollectionDate getter
//-----------------------------------------------------------------------------
void Camera::getDataCollectionDate(std::string& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::DATA_COLLECTION_DATE,value);
}

//-----------------------------------------------------------------------------
///  SoftwareVersion getter
//-----------------------------------------------------------------------------
void Camera::getSoftwareVersion(std::string& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  value = m_software_version;
}
            
//-----------------------------------------------------------------------------
//- Compression getter (only for FileWriter mode)
//-----------------------------------------------------------------------------
void Camera::getCompression(bool& value) ///< [out] true:enabled, false:disabled
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::FILEWRITER_COMPRESSION,value);
}


//-----------------------------------------------------------------------------
//- Compression setter (only for FileWriter mode)
//-----------------------------------------------------------------------------
void Camera::setCompression(bool value)
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_SET_PARAM(Requests::FILEWRITER_COMPRESSION,value);
}

//-----------------------------------------------------------------------------
//-
//-----------------------------------------------------------------------------
void Camera::getCompressionType(Camera::CompressionType& type) const
{
  DEB_MEMBER_FUNCT();
  std::string compression_type;
  EIGER_SYNC_GET_PARAM(Requests::COMPRESSION_TYPE,compression_type);
  DEB_RETURN() << DEB_VAR1(compression_type);
  type = compression_type == "lz4" ? LZ4 : BSLZ4;
}

//-----------------------------------------------------------------------------
//-
//-----------------------------------------------------------------------------
void Camera::setCompressionType(Camera::CompressionType type)
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_SET_PARAM(Requests::COMPRESSION_TYPE,
		       type == LZ4 ? "lz4" : "bslz4");
}

//-----------------------------------------------------------------------------
//-
//-----------------------------------------------------------------------------
void Camera::getSerieId(int& serie_id)
{
  DEB_MEMBER_FUNCT();
  serie_id = m_serie_id;
  DEB_RETURN() << DEB_VAR1(serie_id);
}

//-----------------------------------------------------------------------------
//-
//-----------------------------------------------------------------------------
void Camera::deleteMemoryFiles()
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_CMD(Requests::FILEWRITER_CLEAR);
}

//-----------------------------------------------------------------------------
//-
//-----------------------------------------------------------------------------
void Camera::disarm()
{
  DEB_MEMBER_FUNCT();
  DEB_TRACE()<<"Camera::disarm() : DISARM";
  EIGER_SYNC_CMD(Requests::DISARM);
}

//-----------------------------------------------------------------------------
//-
//-----------------------------------------------------------------------------
const std::string& Camera::getDetectorIp() const
{
  return m_detector_ip;
}

//-----------------------------------------------------------------------------
//-
//-----------------------------------------------------------------------------
const std::string& Camera::getTimestampType() const
{
  return m_timestamp_type;
}

//-----------------------------------------------------------------------------
//-
//-----------------------------------------------------------------------------
void  Camera::setTimestampType(const std::string& timestamp)
{
	m_timestamp_type = timestamp;
}


//-----------------------------------------------------------------------------
//-
//-----------------------------------------------------------------------------
void  Camera::setCurlDelayMs(double curl_delay_ms)
{
    m_requests->set_curl_delay_ms(curl_delay_ms);
}


//-----------------------------------------------------------------------------
///  getDetectorReadoutTime getter
//-----------------------------------------------------------------------------
void Camera::getDetectorReadoutTime(double& value) ///< [out] 
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_GET_PARAM(Requests::DETECTOR_READOUT_TIME,value);
}

//-----------------------------------------------------------------------------
///  setRoiMode getter
//-----------------------------------------------------------------------------
void Camera::setRoiMode(const std::string& value)
{
  DEB_MEMBER_FUNCT();
  EIGER_SYNC_SET_PARAM(Requests::ROI_MODE, value);	
}

//-----------------------------------------------------------------------------
///  getRoiMode getter
//-----------------------------------------------------------------------------
void Camera::getRoiMode(std::string& value)
{
  DEB_MEMBER_FUNCT();	
  std::string mode;
  EIGER_SYNC_GET_PARAM(Requests::ROI_MODE, mode);
  DEB_RETURN() << DEB_VAR1(mode);
  value = mode;
}