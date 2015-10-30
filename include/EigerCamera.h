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
#ifndef EIGERCAMERA_H
#define EIGERCAMERA_H

#include "EigerCompatibility.h"
#if defined (__GNUC__) && (__GNUC__ == 3) && defined (__ELF__)
#   define GENAPI_DECL __attribute__((visibility("default")))
#   define GENAPI_DECL_ABSTRACT __attribute__((visibility("default")))
#endif


#include <stdlib.h>
#include <limits>
#include "lima/HwMaxImageSizeCallback.h"
#include "lima/HwBufferMgr.h"
#include "lima/ThreadUtils.h"

#include <EigerAdapter.h>

#include <ostream>

using namespace std;

#define HANDLE_EIGERERROR(__errMsg__) { \
										DEB_ERROR() << __errMsg__;\
										THROW_HW_ERROR(Error) << __errMsg__;\
									  }

#define EIGER_EXEC(__EigerCode__) {\
									try\
									{\
										__EigerCode__;\
									}\
									catch (const eigerapi::EigerException &e)\
								    {\
								        HANDLE_EIGERERROR(e.what());\
								    }\
								  }

// Delay between two getStatus calls
#define C_DETECTOR_POLL_TIME 2  // second
#define C_DETECTOR_MAX_TIME  60 // seconds

// Name of the downloaded file
#define DOWNLOADED_MASTER_FILE_NAME "/temp_master.h5"
#define DOWNLOADED_DATA_FILE_NAME "/temp_data.h5"
namespace lima
{
   namespace Eiger
   {

   /*******************************************************************
   * \class Camera
   * \brief object controlling the Eiger camera via EigerAPI
   *******************************************************************/
	class LIBEIGER Camera
	{
		DEB_CLASS_NAMESPC(DebModCamera, "Camera", "Eiger");
		friend class Interface;

		public:

			enum Status {	Ready, Exposure, Readout, Latency, Fault, Preparing, Armed };

			Camera(const std::string& detector_ip, const std::string& target_path);
			~Camera();

			void startAcq();
			void stopAcq();
			void prepareAcq();

			// -- detector info object
			void getImageType(ImageType& type);
			void setImageType(ImageType type);

			void getDetectorType(std::string& type);
			void getDetectorModel(std::string& model);
			void getDetectorImageSize(Size& size);
			void getDetectorMaxImageSize(Size& size);

			// -- Buffer control object
			HwBufferCtrlObj* getBufferCtrlObj();

			//-- Synch control object
			bool checkTrigMode(TrigMode trig_mode);
			void setTrigMode(TrigMode  mode);
			void getTrigMode(TrigMode& mode);

			void setExpTime(double  exp_time);
			void getExpTime(double& exp_time);

			void setLatTime(double  lat_time);
			void getLatTime(double& lat_time);

			void getExposureTimeRange(double& min_expo, double& max_expo) const;
			void getLatTimeRange(double& min_lat, double& max_lat) const;

			void setNbFrames(int  nb_frames);
			void getNbFrames(int& nb_frames);
			void getNbHwAcquiredFrames(int &nb_acq_frames);

			bool isBinningAvailable();

			void getPixelSize(double& sizex, double& sizey);

			Camera::Status getStatus();

//			void reset();

			// -- Eiger specific
			double getTemperature();
			double getHumidity();
            void setFileNamePattern(const std::string& pattern);
			void setCountrateCorrection(const bool);
			void getCountrateCorrection(bool&);
			void setFlatfieldCorrection(const bool);
			void getFlatfieldCorrection(bool&);
		    void setEfficiencyCorrection(const bool);
		    void getEfficiencyCorrection(bool& value);
			void setPixelMask(const bool);
			void getPixelMask(bool&);
			void setThresholdEnergy(const double);
			void getThresholdEnergy(double&);
			void setVirtualPixelCorrection(const bool);
			void getVirtualPixelCorrection(bool&);
			void setPhotonEnergy(const double);
			void getPhotonEnergy(double&);			

			void getCompression(bool&);
   			void setCompression(const bool);
            
            bool getReaderHDF5();
            void setReaderHDF5(const bool);
            void setVerbosity(const bool verbose){m_eiger_adapter->setVerbosity(verbose);}
		private:
			class CameraThread: public CmdThread
			{
				DEB_CLASS_NAMESPC(DebModCamera, "CameraThread", "Eiger");

				public:

				enum
				{ // thread Status
					Ready = MaxThreadStatus, Exposure, Readout, Latency, Fault, Preparing, Armed
				};

				enum
				{ // thread commands
					StartAcq = MaxThreadCmd,
					StopAcq,
					PrepareAcq
				};

				CameraThread(Camera& cam);

				virtual void start();

				volatile bool m_force_stop;

				protected:
					virtual void init();
					virtual void execCmd(int cmd);

				private:
					void execStartAcq();
					void execPrepareAcq();
                    void WaitForState(eigerapi::ENUM_STATE eTargetStateDET,  ///< [in] Detector state to wait for
                                      eigerapi::ENUM_STATE eTargetStateFW);  ///< [in] Filewriter state to wait for
					
               Camera* m_cam;
			};
			friend class CameraThread;

			
			void initialiseController(); /// Used during plug-in initialization
			eigerapi::ENUM_TRIGGERMODE getTriggerMode(const TrigMode trig_mode); ///< [in] lima trigger mode value
			bool isBinningSupported(const int binValue);	/// Check if a binning value is supported            
            
            // Chronometers functions
            void    resetChrono();
            double  elapsedChrono();
        	Timestamp   m_chrono_0;
            Timestamp   m_chrono_1;  
            //////////////////////

			//-----------------------------------------------------------------------------
			//- lima stuff
			SoftBufferCtrlObj	      m_buffer_ctrl_obj;
			int                       m_nb_frames;
			Camera::Status            m_status;
			int                       m_image_number;
			double                    m_latency_time;
			TrigMode                  m_trig_mode;

			//- camera stuff
			string                    m_detector_model;
			string                    m_detector_type;
			long					  m_max_image_width;
            long                      m_max_image_height;
            ImageType                 m_detector_image_type;  

			//- EigerAPI stuff
         	eigerapi::EigerAdapter*   m_eiger_adapter;
         
			double                    m_temperature;
			double                    m_humidity;
			map<TrigMode, eigerapi::ENUM_TRIGGERMODE> m_map_trig_modes;
			double                    m_exp_time;
			double                    m_exp_time_max;
			double                    m_x_pixel_size; 
            double                    m_y_pixel_size;
            string                    m_target_path;            
            bool                      m_is_reader_hdf5_enabled;
			CameraThread 			  m_thread;
	};
	} // namespace Eiger
} // namespace lima


#endif // EIGERCAMERA_H
