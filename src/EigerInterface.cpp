//###########################################################################
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
//###########################################################################
#include "EigerInterface.h"
#include "EigerCamera.h"
#include "EigerDetInfoCtrlObj.h"
#include "EigerSyncCtrlObj.h"

using namespace lima;
using namespace lima::Eiger;
using namespace std;


//-----------------------------------------------------
// @brief Ctor
//-----------------------------------------------------
Interface::Interface(Camera& cam) : m_cam(cam) 
{
    DEB_CONSTRUCTOR();
    m_det_info = new DetInfoCtrlObj(cam);
	m_sync     = new SyncCtrlObj(cam);
	
    m_cap_list.push_back(HwCap(m_det_info));
    
	HwBufferCtrlObj* buffer = m_cam.getBufferCtrlObj();
	m_cap_list.push_back(HwCap(buffer));	
    
	m_cap_list.push_back(HwCap(m_sync));
}

//-----------------------------------------------------
// @brief Dtor
//-----------------------------------------------------
Interface::~Interface()
{
    DEB_DESTRUCTOR();
	delete m_det_info;
	delete m_sync;	
}

//-----------------------------------------------------
// @brief return the capability list
//-----------------------------------------------------
void Interface::getCapList(HwInterface::CapList &cap_list) const
{
    DEB_MEMBER_FUNCT();
    cap_list = m_cap_list;
}

//-----------------------------------------------------
// @brief reset the interface, stop the acqisition
//-----------------------------------------------------
void Interface::reset(ResetLevel reset_level)
{
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(reset_level);

    stopAcq();
}

//-----------------------------------------------------
// @brief do nothing
//-----------------------------------------------------
void Interface::prepareAcq()
{
    DEB_MEMBER_FUNCT();
	m_cam.prepareAcq();
}

//-----------------------------------------------------
// @brief start the camera acquisition
//-----------------------------------------------------
void Interface::startAcq()
{
    DEB_MEMBER_FUNCT();
    m_cam.startAcq();
}

//-----------------------------------------------------
// @brief stop the camera acquisition
//-----------------------------------------------------
void Interface::stopAcq()
{
  DEB_MEMBER_FUNCT();
  m_cam.stopAcq();
}

//-----------------------------------------------------
// @brief return the status of detector/acquisition
//-----------------------------------------------------
void Interface::getStatus(StatusType& status)
{
    DEB_MEMBER_FUNCT();
    
    Camera::Status Eiger_status = Camera::Ready;
    Eiger_status = m_cam.getStatus();
    switch (Eiger_status)
    {
      case Camera::Ready:
        status.set(HwInterface::StatusType::Ready);
        break;

      case Camera::Exposure:
        status.set(HwInterface::StatusType::Exposure);
        break;

      case Camera::Readout:
        status.set(HwInterface::StatusType::Readout);
        break;

      case Camera::Latency:
        status.set(HwInterface::StatusType::Latency);
        break;

      case Camera::Fault:
        status.set(HwInterface::StatusType::Fault);
        break;
        
      case Camera::Preparing:
         status.set(HwInterface::StatusType::Exposure);
    }
    
    DEB_RETURN() << DEB_VAR1(status);
}


//-----------------------------------------------------
// @brief return the hw number of acquired frames
//-----------------------------------------------------
int Interface::getNbHwAcquiredFrames()
{
     DEB_MEMBER_FUNCT();
     int acq_frames;
     m_cam.getNbHwAcquiredFrames(acq_frames);
     return acq_frames;
}

