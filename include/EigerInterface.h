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
#ifndef EIGERINTERFACE_H
#define EIGERINTERFACE_H

#include "EigerCompatibility.h"
#include "HwInterface.h"
/*#include "EigerCamera.h"
#include "EigerDetInfoCtrlObj.h"
#include "EigerSyncCtrlObj.h"*/

namespace lima
{
    namespace Eiger
    {

	class DetInfoCtrlObj;
    class SyncCtrlObj;
    class Camera;
	
	/*******************************************************************
	* \class Interface
	* \brief Eiger hardware interface
	*******************************************************************/
	class LIBEIGER Interface : public HwInterface
	{
	DEB_CLASS_NAMESPC(DebModCamera, "EigerInterface", "Eiger");

	public:
	    Interface(Camera& cam);
	    virtual ~Interface();

	    //- From HwInterface
	    virtual void    getCapList(CapList&) const;
	    virtual void    reset(ResetLevel reset_level);
	    virtual void    prepareAcq();
	    virtual void    startAcq();
	    virtual void    stopAcq();
	    virtual void    getStatus(StatusType& status);
	    virtual int     getNbHwAcquiredFrames();
    
		//! get the camera object to access it directly from client
		Camera& getCamera() { return m_cam;}

	private:
	    Camera&         m_cam;
	    CapList         m_cap_list;
	    DetInfoCtrlObj* m_det_info;
	    SyncCtrlObj*    m_sync;
	};

    } // namespace Eiger
} // namespace lima

#endif // EigerINTERFACE_H
