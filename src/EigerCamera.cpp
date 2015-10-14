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
#include "lima/Timestamp.h"

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
        {
            if (status != Armed)
            {
                DEB_ERROR() << "CameraThread::execCmd - Not Ready to StartAcq";
                throw LIMA_HW_EXC(InvalidValue, "Not Ready to StartAcq");
            }
            execStartAcq();
            break;
        }

        case PrepareAcq:
        {
            if (status != Ready)
            {
                DEB_ERROR() << "CameraThread::execCmd - Not Ready to PrepareAcq";
                throw LIMA_HW_EXC(InvalidValue, "Not Ready to PrepareAcq");
            }
            execPrepareAcq();
            break;
        }

        default:
        {
            DEB_ERROR() << "CameraThread::execCmd  Unknown command.";
            THROW_HW_ERROR(Error) << "CameraThread::execCmd  Unknown command.";
        }
    }
    DEB_TRACE() << "CameraThread::execCmd - END";
}


//-----------------------------------------------------------------------------
///  Waits for a specific device state
//-----------------------------------------------------------------------------
void Camera::CameraThread::WaitForState(eigerapi::ENUM_STATE eTargetStateDET, ///< [in] Detector state to wait for
                                        eigerapi::ENUM_STATE eTargetStateFW)  ///< [in] Filewriter state to wait for
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "CameraThread::WaitForState - BEGIN";

    int iterCount = 0; // iteration counter

    eigerapi::ENUM_STATE eStateFileWriter, eStateDetector;
    do
    {
        try
        {
            eStateFileWriter = m_cam->m_eiger_adapter->getState(eigerapi::ESUBSYSTEM_FILEWRITER);
            eStateDetector   = m_cam->m_eiger_adapter->getState(eigerapi::ESUBSYSTEM_DETECTOR);
        }
        catch (const eigerapi::EigerException &e)
        {
            HANDLE_EIGERERROR(e.what());
        }

        iterCount++;

        // Check for faulty states
        bool bError = false;
        switch (eStateFileWriter)
        {
            case eigerapi::ESTATE_DISABLED:
            case eigerapi::ESTATE_ERROR:
                bError = true;
                break;
        }
        switch (eStateDetector)
        {
            case eigerapi::ESTATE_DISABLED:
            case eigerapi::ESTATE_ERROR:
                bError = true;
                break;
        }

        if (bError)
        {
            HANDLE_EIGERERROR("Faulty state reached during WaitForState");
        }

        // Check for operation timeout
        if ( (eTargetStateFW!=eStateFileWriter) && (eTargetStateDET!=eStateDetector) && (iterCount*C_DETECTOR_POLL_TIME >= C_DETECTOR_MAX_TIME) )
        {
            HANDLE_EIGERERROR("Timeout reached during WaitForState.");
        }

        // If target state still not reached, wait before requesting the state again
        if ( (eTargetStateFW!=eStateFileWriter) || (eTargetStateDET!=eStateDetector) )
        {
            lima::Sleep(C_DETECTOR_POLL_TIME);
        }
    }
    while ( (eTargetStateFW!=eStateFileWriter) || (eTargetStateDET!=eStateDetector) );

    // test: an aditional state always called ...
    lima::Sleep(C_DETECTOR_POLL_TIME);

    DEB_TRACE() << "CameraThread::WaitForState - END";
}


//---------------------------------------------------------------------------------------
//! Thread prepare the acquisition by sending the arm command
//---------------------------------------------------------------------------------------
void Camera::CameraThread::execPrepareAcq()
{
    DEB_MEMBER_FUNCT();

    DEB_TRACE() << "CameraThread::execPrepareAcq - BEGIN";

    setStatus(Preparing);

    // send the arm command
    m_cam->resetChrono();
    try
    {
        m_cam->m_eiger_adapter->arm();
    }
    catch (const eigerapi::EigerException &e)
    {
        DEB_ERROR() << e.what();
        setStatus(Fault);
        return;
    }

    DEB_TRACE() << "Duration: " << m_cam->elapsedChrono() << "s";

    setStatus(Armed);

    DEB_TRACE() << "CameraThread::execPrepareAcq - END";
}


//---------------------------------------------------------------------------------------
//! Thread capture function for default capture mode
//---------------------------------------------------------------------------------------
void Camera::CameraThread::execStartAcq()
{
    DEB_MEMBER_FUNCT();

    DEB_TRACE() << "CameraThread::execStartAcq - BEGIN";

    setStatus(Readout);

    StdBufferCbMgr& buffer_mgr = m_cam->m_buffer_ctrl_obj.getBuffer();

    FrameDim frame_dim = buffer_mgr.getFrameDim();
    Size frame_size    = frame_dim.getSize();
    int height         = frame_size.getHeight();
    int width          = frame_size.getWidth();

    //-------------------------------------------------- 
    // 1 - send the trigger command (should return only when acquisition ended)
    //-------------------------------------------------- 
    DEB_TRACE() << " ";
    DEB_TRACE() << "Trigger to start the acquisition (synchronous!) ...";
    DEB_TRACE() << "------------------------------------------------";
    m_cam->resetChrono();
    try
    {
        m_cam->m_eiger_adapter->trigger();
    }
    catch (const eigerapi::EigerException &e)
    {
        DEB_ERROR() << e.what();
        setStatus(Fault);
        return;
    }

    // display trigger duration in s (DEBUG_INFO)
    DEB_TRACE() << "Duration trigger : " << m_cam->elapsedChrono() << "s";

    //-------------------------------------------------- 
    // 2 - Send a disarm command to finalize the acquisition
    //-------------------------------------------------- 
    DEB_TRACE() << " ";
    DEB_TRACE() << "Disarm to finalize the acquisition ...";
    DEB_TRACE() << "------------------------------------------------";
    m_cam->resetChrono();
    try
    {
        m_cam->m_eiger_adapter->disarm();
    }
    catch (const eigerapi::EigerException &e)
    {
        DEB_ERROR() << e.what();
        setStatus(Fault);
        return;
    }

    // display disarm duration in s (DEBUG_INFO)
    DEB_TRACE() << "Duration disarm : " << m_cam->elapsedChrono() << "s";

    //--------------------------------------------------  
    // 3 - Wait for filewriter status to be ready
    //-------------------------------------------------- 
    try
    {
        WaitForState(eigerapi::ESTATE_IDLE, eigerapi::ESTATE_READY);
    }
    catch (Exception& e)
    {
        setStatus(Fault);
        return;
    }

    //-------------------------------------------------- 
    // (TANGODEVIC-1256)
    // 4 - Download master file
    //--------------------------------------------------
    DEB_TRACE() << " ";
    DEB_TRACE() << "Download master file ...";
    DEB_TRACE() << "------------------------------------------------";
    m_cam->resetChrono();
    try
    {   // This call must be commented when testing with the Eiger server Simulator 1.0
        m_cam->m_eiger_adapter->downloadMasterFile(m_cam->m_target_master_file_name);
    }
    catch (const eigerapi::EigerException &e)
    {
        setStatus(Fault);
        DEB_ERROR() << e.what();
        return;
    }

    // display data transfer rate in MB/s (DEBUG_INFO)
    double download_duration_master_file = m_cam->elapsedChrono();
    long size_master_file = m_cam->getFileSize(m_cam->m_target_master_file_name);
    DEB_TRACE() <<  "Size         : " << size_master_file << " bytes";
    DEB_TRACE() <<  "Download Duration     : " << download_duration_master_file << "s";
    if (download_duration_master_file > 0.0)
    {
        DEB_TRACE() <<  "Speed        : " << size_master_file / 1000000 / download_duration_master_file << " MB/s";
    }

    //--------------------------------------------------
    // 5 - Download and open the captured file for reading
    //--------------------------------------------------
    DEB_TRACE() << " ";
    DEB_TRACE() << "Download data file ...";
    DEB_TRACE() << "------------------------------------------------";
    m_cam->resetChrono();
    try
    {   // This call must be commented when testing with the Eiger server Simulator 1.0
        m_cam->m_eiger_adapter->downloadAcquiredFile(m_cam->m_target_data_file_name);
    }
    catch (const eigerapi::EigerException &e)
    {
        setStatus(Fault);
        DEB_ERROR() << e.what();
        return;
    }

    // display data transfer rate in MB/s    (DEBUG_INFO)
    double download_duration_data_file = m_cam->elapsedChrono();
    long size_data_file = m_cam->getFileSize(m_cam->m_target_data_file_name);
    DEB_TRACE() <<  "Size         : " << size_data_file << " bytes";
    DEB_TRACE() <<  "Download Duration     : " << download_duration_data_file << "s";
    if (download_duration_data_file > 0.0)
    {
        DEB_TRACE() <<  "Speed        : " << size_data_file / 1000000 / download_duration_data_file << " MB/s";
    }

    if(m_cam->getReaderHDF5())
    {
        //--------------------------------------------------
        // 6 - Open the captured file for reading
        //--------------------------------------------------
        DEB_TRACE() << " ";
        DEB_TRACE() << "Open the HDF5 data file ...";
        DEB_TRACE() << "------------------------------------------------";
        m_cam->resetChrono();
        try
        {
            m_cam->m_eiger_adapter->openDataFile(m_cam->m_target_data_file_name);
        }
        catch (const eigerapi::EigerException &e)
        {
            setStatus(Fault);
            DEB_ERROR() << e.what();
            return;
        }

        // display open file rate in MB/s    (DEBUG_INFO)
        double open_duration_data_file = m_cam->elapsedChrono();
        DEB_TRACE() <<  "Size         : " << size_data_file << " bytes";
        DEB_TRACE() <<  "Open Duration     : " << open_duration_data_file << "s";
        if (open_duration_data_file > 0.0)
        {
            DEB_TRACE() <<  "Speed        : " << size_data_file / 1000000 / open_duration_data_file << " MB/s";
        }
    }
    
    //--------------------------------------------------
    // 7 - Begin to transfer the images to Lima
    //--------------------------------------------------
    buffer_mgr.setStartTimestamp(Timestamp::now());
    DEB_TRACE() << " ";
    DEB_TRACE() << "Transfering frames to Lima ...";
    DEB_TRACE() << "------------------------------------------------";
    // Acquisition loop
    m_cam->resetChrono();
    bool continueAcq = true;
    while ( continueAcq && (!m_cam->m_nb_frames || m_cam->m_image_number < m_cam->m_nb_frames) )
    {
        // Check if acquisition was stopped by device command & abort the current acquisition
        if (m_force_stop)
        {
            m_force_stop = false;
            continueAcq = false;
            break;
        }

        // Get the next image from the data file
        void* src = m_cam->m_eiger_adapter->getFrame();

        // Get a new frame buffer
        DEB_TRACE() << "m_image_number = " << m_cam->m_image_number;
        void *dst = buffer_mgr.getFrameBufferPtr(m_cam->m_image_number);

        HwFrameInfoType frame_info;
        frame_info.acq_frame_nb = m_cam->m_image_number;
        if (NULL!=src)
        {
            memcpy(dst, src, width * height *2);    //16 bits
        }
        else
        {
            //(TANGODEVIC-1256)
            if(m_cam->getReaderHDF5())
            {
                // no more images are available in the data file !!
                continueAcq = false;
                continue;
            }
            memset(dst, 0, width * height *2);      //16 bits
        }
                
        if (buffer_mgr.newFrameReady(frame_info))
        {
            ++m_cam->m_image_number;
        }
        else
        {
            setStatus(Fault);
            DEB_ERROR() << "newFrameReady failure.";
            continueAcq = false;
            return;
        }
    } /* end of acquisition loop */

    try
    {
        // Delete the acquired file from Eiger server data storage
        DEB_TRACE() << " ";
        DEB_ERROR() << "Deleting data file ...";
        DEB_TRACE() << "------------------------------------------------";
        m_cam->m_eiger_adapter->deleteAcquiredFile();

        // (TANGODEVIC-1256)
        // Delete the master file . Is it reasonable ???        
        DEB_TRACE() << " ";
        DEB_ERROR() << "Deleting master file ...";
        DEB_TRACE() << "------------------------------------------------";
        m_cam->m_eiger_adapter->deleteMasterFile();
    }
    catch (const eigerapi::EigerException &e)
    {
        setStatus(Fault);
        DEB_ERROR() << e.what();
        return;
    }

    DEB_TRACE() << " ";    
    DEB_TRACE() << "Duration of reading and publishing "<<m_cam->m_nb_frames<<" frames : "<<m_cam->elapsedChrono() << "s" ;

    setStatus(Ready);

    DEB_TRACE() << "CameraThread::execStartAcq - END";
}


//-----------------------------------------------------------------------------
///  Ctor
//-----------------------------------------------------------------------------
Camera::Camera(const std::string& detector_ip,	///< [in] Ip address of the detector server
               const std::string& target_path)	///< [in] temporary path where to store downloaded files
: m_thread(*this)
{
    DEB_CONSTRUCTOR();
    m_image_number  = 0;
    m_latency_time  = 0.;
    m_exp_time      = 1.;
    m_eiger_adapter = NULL;
    m_detector_image_type = Bpp16;
    m_target_data_file_name = target_path + DOWNLOADED_DATA_FILE_NAME;
    m_target_master_file_name   = target_path + DOWNLOADED_MASTER_FILE_NAME;
    m_is_reader_hdf5_enabled    = true;
    DEB_TRACE() << "detector_ip = " << detector_ip;
    DEB_TRACE() << "target_path = " << target_path;
    DEB_TRACE() << "target_data_file_name = " << m_target_data_file_name;
    DEB_TRACE() << "target_master_file_name = " << m_target_master_file_name ;
    DEB_TRACE() << "is_reader_hdf5_enabled = " << m_is_reader_hdf5_enabled ;

    // Init EigerAPI
    try
    {
        m_eiger_adapter = new eigerapi::EigerAdapter(detector_ip);

        // --- Initialise deeper parameters of the controller
        initialiseController();
    }
    catch (const eigerapi::EigerException &e)
    {
        HANDLE_EIGERERROR(e.what());
    }

    // Display max image size
    DEB_TRACE() << "Detector max width : " << m_max_image_width;
    DEB_TRACE() << "Detector max height :" << m_max_image_height;

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

    m_thread.abort();

    try
    {
        delete m_eiger_adapter;
    }
    catch (const eigerapi::EigerException &e)
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

    DEB_TRACE() << "Camera::prepareAcq() start CmdThread";

    if (m_thread.getStatus() == CameraThread::Armed)
    {
        DEB_ERROR() << "CameraThread::execCmd - Already Prepared";
        throw LIMA_HW_EXC(InvalidValue, "Already Prepared");
    }

    m_thread.sendCmd(CameraThread::PrepareAcq);
}


//-----------------------------------------------------------------------------
///  start the acquisition
//-----------------------------------------------------------------------------
void Camera::startAcq()
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "Camera::startAcq() start CmdThread";

    // init force stop flag before starting acq thread
    m_thread.m_force_stop = false;

    // Start the thread
    m_image_number = 0;

    m_thread.waitStatus(CameraThread::Armed); // Wait then end of execPrepareAcq()

    m_thread.sendCmd(CameraThread::StartAcq);
}


//-----------------------------------------------------------------------------
/// stop the acquisition
//-----------------------------------------------------------------------------
void Camera::stopAcq()
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "Camera::stopAcq() stop CmdThread" ;

    EIGER_EXEC(m_eiger_adapter->disarm());

    if (m_thread.getStatus() == CameraThread::Readout)
    {
        m_thread.m_force_stop = true;

        // Wait for thread to finish
        m_thread.waitNotStatus(CameraThread::Readout);
    }
}


//-----------------------------------------------------------------------------
/// return the detector Max image size 
//-----------------------------------------------------------------------------
void Camera::getDetectorMaxImageSize(Size& size) ///< [out] image dimensions
{
    DEB_MEMBER_FUNCT();
    size = Size(m_max_image_width, m_max_image_height);
}


//-----------------------------------------------------------------------------
/// return the detector image size 
//-----------------------------------------------------------------------------
void Camera::getDetectorImageSize(Size& size) ///< [out] image dimensions
{
    DEB_MEMBER_FUNCT();

    eigerapi::EigerSize eSz;
    EIGER_EXEC(eSz = m_eiger_adapter->getDetectorSize());
    int width  = eSz.getX();
    int height = eSz.getY();

    size = Size(width, height);
}


//-----------------------------------------------------------------------------
/// Get the image type
//-----------------------------------------------------------------------------
void Camera::getImageType(ImageType& type) ///< [out] image type
{
    DEB_MEMBER_FUNCT();

    type = m_detector_image_type;
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
            EIGER_EXEC(m_eiger_adapter->setTriggerMode(eEigertrigMode));
            m_trig_mode = mode;
        }
    }
    catch (const eigerapi::EigerException &e)
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
    EIGER_EXEC(m_eiger_adapter->setExposureTime(exp_time));

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
            m_eiger_adapter->setLatencyTime(lat_time);
            m_latency_time = lat_time;
        }
    }
    catch (const eigerapi::EigerException &e)
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
    min_expo = 0.0001;
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

    try
    {
        m_eiger_adapter->setNbImagesToAcquire(nb_frames);
        m_nb_frames = nb_frames;
    }
    catch (const eigerapi::EigerException &e)
    {
        HANDLE_EIGERERROR(e.what());
    }
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

        case CameraThread::Preparing:
            return Camera::Preparing;

        case CameraThread::Armed:
            return Camera::Armed;

        case CameraThread::Fault:
            return Camera::Fault;

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

    sizex = m_x_pixel_size;
    sizey = m_y_pixel_size;
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

    // Fills the map of available trigger modes
    m_map_trig_modes[IntTrig] 		= eigerapi::ETRIGMODE_EXPO;
    m_map_trig_modes[ExtTrigSingle] = eigerapi::ETRIGMODE_EXTT;
    m_map_trig_modes[ExtTrigMult]	= eigerapi::ETRIGMODE_EXTM;
    m_map_trig_modes[ExtGate]       = eigerapi::ETRIGMODE_EXTE;

    // Retrieve pixel size
    DEB_TRACE() << "m_eiger_adapter->getPixelSize()";
    eigerapi::EigerSize eSz = m_eiger_adapter->getPixelSize();
    m_x_pixel_size = eSz.getX();
    m_y_pixel_size = eSz.getY();


    Size sizeMax;
    getDetectorImageSize(sizeMax);

    // Store max image size
    m_max_image_width  = sizeMax.getWidth();
    m_max_image_height = sizeMax.getHeight();

    // Store pixel depth    
    int bits = m_eiger_adapter->getBitDepthReadout();
    switch (bits)
    {
        case 12: m_detector_image_type = Bpp16;
            break;
        default:
        {
            char Msg[256];
            sprintf(Msg, "Unexpected bit depth: %d", bits);
            HANDLE_EIGERERROR(Msg);
        }
    }

    // Detector model
    DEB_TRACE() << "m_eiger_adapter->getDescription()";
    m_detector_model = m_eiger_adapter->getDescription();

    // Detector number
    DEB_TRACE() << "m_eiger_adapter->getDetectorNumber";
    m_detector_type = m_eiger_adapter->getDetectorNumber();

    // Retrieve exposure time
    DEB_TRACE() << "m_eiger_adapter->getExposureTime()";
    m_exp_time = m_eiger_adapter->getExposureTime();

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
    ////m_eiger_adapter->status_update();
    EIGER_EXEC(return m_eiger_adapter->getTemperature());
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
    ////m_eiger_adapter->status_update();
    EIGER_EXEC(return m_eiger_adapter->getHumidity());
}


//-----------------------------------------------------------------------------
///  Count rate correction setter
//-----------------------------------------------------------------------------
void Camera::setCountrateCorrection(const bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_eiger_adapter->setCountrateCorrection(value));
}


//-----------------------------------------------------------------------------
///  Count rate correction getter
//-----------------------------------------------------------------------------
void Camera::getCountrateCorrection(bool& value)  ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_eiger_adapter->getCountrateCorrection());
}


//-----------------------------------------------------------------------------
///  FlatfieldCorrection setter
//-----------------------------------------------------------------------------
void Camera::setFlatfieldCorrection(const bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_eiger_adapter->setFlatfieldCorrection(value));
}


//-----------------------------------------------------------------------------
///  FlatfieldCorrection getter
//-----------------------------------------------------------------------------
void Camera::getFlatfieldCorrection(bool& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_eiger_adapter->getFlatfieldCorrection());
}


//-----------------------------------------------------------------------------
///  PixelMask setter
//-----------------------------------------------------------------------------
void Camera::setPixelMask(const bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_eiger_adapter->setPixelMask(value));
}


//-----------------------------------------------------------------------------
///  PixelMask getter
//-----------------------------------------------------------------------------
void Camera::getPixelMask(bool& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_eiger_adapter->getPixelMask());
}

//-----------------------------------------------------------------------------
/// EfficiencyCorrection setter
//-----------------------------------------------------------------------------
void Camera::setEfficiencyCorrection(const bool enabled) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_eiger_adapter->setEfficiencyCorrection(enabled));
}


//-----------------------------------------------------------------------------
/// EfficiencyCorrection getter
//-----------------------------------------------------------------------------
void Camera::getEfficiencyCorrection(bool& value)  ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_eiger_adapter->getEfficiencyCorrection());
}


//-----------------------------------------------------------------------------
///  ThresholdEnergy setter
//-----------------------------------------------------------------------------
void Camera::setThresholdEnergy(const double value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_eiger_adapter->setThresholdEnergy(value));
}


//-----------------------------------------------------------------------------
///  ThresholdEnergy getter
//-----------------------------------------------------------------------------
void Camera::getThresholdEnergy(double& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_eiger_adapter->getThresholdEnergy());
}


//-----------------------------------------------------------------------------
///  VirtualPixelCorrection setter
//-----------------------------------------------------------------------------
void Camera::setVirtualPixelCorrection(const bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_eiger_adapter->setVirtualPixelCorrection(value));
}


//-----------------------------------------------------------------------------
///  VirtualPixelCorrection getter
//-----------------------------------------------------------------------------
void Camera::getVirtualPixelCorrection(bool& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_eiger_adapter->getVirtualPixelCorrection());
}


//-----------------------------------------------------------------------------
///  PhotonEnergy setter
//-----------------------------------------------------------------------------
void Camera::setPhotonEnergy(const double value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_eiger_adapter->setPhotonEnergy(value));
}


//-----------------------------------------------------------------------------
///  PhotonEnergy getter
//-----------------------------------------------------------------------------
void Camera::getPhotonEnergy(double& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_eiger_adapter->getPhotonEnergy());
}


//-----------------------------------------------------------------------------
/// Compression status getter
//-----------------------------------------------------------------------------
void Camera::getCompression(bool& value) ///< [out] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(value = m_eiger_adapter->getCompression());
}


//-----------------------------------------------------------------------------
/// Compression status setter
//-----------------------------------------------------------------------------
void Camera::setCompression(const bool value) ///< [in] true:enabled, false:disabled
{
    DEB_MEMBER_FUNCT();

    EIGER_EXEC(m_eiger_adapter->setCompression(value));
}

//-----------------------------------------------------------------------------
/// reader HDF5 setter
//-----------------------------------------------------------------------------
bool Camera::getReaderHDF5()
{
    return m_is_reader_hdf5_enabled;
}

//-----------------------------------------------------------------------------
/// reader HDF5 getter
//-----------------------------------------------------------------------------            
void Camera::setReaderHDF5(const bool value)
{
    m_is_reader_hdf5_enabled = value;
}

//-----------------------------------------------------------------------------
/// Timer start function
//-----------------------------------------------------------------------------
void Camera::resetChrono()
{
    m_chrono_0 = Timestamp::now();
}


//-----------------------------------------------------------------------------
/// Timer stop function
/*
@return elapsed time (seconds)
 */
//-----------------------------------------------------------------------------
double Camera::elapsedChrono()
{
    m_chrono_1 = Timestamp::now();
    return (m_chrono_1 - m_chrono_0);
}

//-----------------------------------------------------------------------------
/// File size getter
/*
@return file size (bytes)
 */
//-----------------------------------------------------------------------------
long Camera::getFileSize(const std::string& fullName) ///< [in] full file name (including path)
{
    FILE *pFile = fopen(fullName.c_str() , "rb");

    // set the file pointer to end of file
    fseek( pFile, 0, SEEK_END );

    // get the file size
    long size = ftell( pFile );

    // close stream and release buffer
    fclose( pFile );

    return size;
}
