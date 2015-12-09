//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2015
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
#ifndef EIGERSTREAM_H
#define EIGERSTREAM_H

#include "lima/Debug.h"

#include "EigerCamera.h"
#include "lima/HwBufferMgr.h"

namespace lima
{
  namespace Eiger
  {
    class Stream
    {
      DEB_CLASS_NAMESPC(DebModCamera,"Stream","Eiger");
    public:
      class Message;
      enum HeaderDetail {ALL,BASIC,OFF};

      Stream(Camera&);
      ~Stream();

      void start();
      void stop();
      bool isRunning() const;
      
      void getHeaderDetail(HeaderDetail&) const;
      void setHeaderDetail(HeaderDetail);
      
      void setActive(bool);
      bool isActive() const;

      HwBufferCtrlObj* getBufferCtrlObj();
      bool get_msg(void* aDataBuffer,void*& msg_data,size_t& msg_size,
		   int& depth);
    private:
      class _BufferCallback;
      class _BufferCtrlObj;
      friend class _BufferCtrlObj;

      static void* _runFunc(void*);
      void _run();
      void _send_synchro();
      
      Camera&		m_cam;
      bool		m_active;
      HeaderDetail	m_header_detail;
      bool		m_dirty_flag;

      mutable Cond	m_cond;
      bool		m_wait;
      bool		m_running;
      bool		m_stop;

      pthread_t		m_thread_id;
      void*		m_zmq_context;
      int		m_pipes[2];
      _BufferCallback*	m_buffer_cbk;
      _BufferCtrlObj*	m_buffer_ctrl_obj;
    };
  }
}
#endif	// EIGERSTREAM_H
