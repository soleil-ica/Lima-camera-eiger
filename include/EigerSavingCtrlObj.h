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
#include <map>
#include "lima/Debug.h"
#include "lima/HwSavingCtrlObj.h"

#include "EigerCamera.h"

namespace lima
{
namespace Eiger
{

class SavingCtrlObj : public HwSavingCtrlObj
{
    DEB_CLASS_NAMESPC(DebModCamera, "SavingCtrlObj", "Eiger");
public:

    enum Status
    {
        IDLE, RUNNING, ERROR
    } ;
    SavingCtrlObj(Camera&);
    virtual ~SavingCtrlObj();

    virtual void getPossibleSaveFormat(std::list<std::string> &format_list) const;

    virtual void setCommonHeader(const HwSavingCtrlObj::HeaderMap&);
    virtual void resetCommonHeader();

    void setSerieId(int value);
    Status getStatus();
    void stop();
	void setDownloadDataFile(bool must_download);    
protected:
    class _PollingThread;
    friend class _PollingThread;
    class _EndDownloadCallback;
    friend class _EndDownloadCallback;

    virtual void _prepare(int = 0);
    virtual void _start(int = 0);
    virtual void _setActive(bool, int = 0);

    Camera&			m_cam;
    int             m_serie_id;
    int             m_nb_file_to_watch;
    int             m_nb_file_transfer_started;
    int             m_concurrent_download;
    bool			m_poll_master_file;
    bool            m_must_download_data_file;
    double			m_waiting_time;
    std::string		m_error_msg;
    //Synchro
    Cond			m_cond;
    bool			m_quit;
    _PollingThread*		m_polling_thread;
    std::map<std::string, int>	m_availables_header_keys;
} ;
}
}
