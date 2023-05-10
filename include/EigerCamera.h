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
#include "lima/ThreadUtils.h"
#include "lima/Event.h"
#include "lima/Timestamp.h"
#include <eigerapi/EigerDefines.h>

#include <ostream>

namespace eigerapi
{
  class Requests;
}

namespace lima
{
   namespace Eiger
   {
     class SavingCtrlObj;
     class Stream;
   /*******************************************************************
   * \class Camera
   * \brief object controlling the Eiger camera via EigerAPI
   *******************************************************************/
     class LIBEIGER Camera : public HwMaxImageSizeCallbackGen, public EventCallbackGen
	{
		DEB_CLASS_NAMESPC(DebModCamera, "Camera", "Eiger");
		friend class Interface;
		friend class SavingCtrlObj;
		friend class Stream;

		public:

		enum Status { Ready, Initialising, Exposure, Readout, Fault };
		enum CompressionType {LZ4,BSLZ4};

			Camera(const std::string& detector_ip);
			~Camera();

			void initialize();

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
			std::string getCamStatus();
//			void reset();

			// -- Eiger specific
			void getTemperature(double&);
			void getHumidity(double&);
         
			void setCountrateCorrection(bool);
			void getCountrateCorrection(bool&);
			void setFlatfieldCorrection(bool);
			void getFlatfieldCorrection(bool&);
			void setAutoSummation(bool);
			void getAutoSummation(bool&);
		    void setEfficiencyCorrection(bool);
		    void getEfficiencyCorrection(bool& value);
			void setPixelMask(bool);
			void getPixelMask(bool&);
			void setThresholdEnergy(double);
			void getThresholdEnergy(double&);
			void setVirtualPixelCorrection(bool);
			void getVirtualPixelCorrection(bool&);
			void setPhotonEnergy(double);
            void getPhotonEnergy(double&);		

            void getDataCollectionDate(std::string&);                        
            void getSoftwareVersion(std::string&);

            //- used for the header of the file
            void setWavelength(double);
            void getWavelength(double&);
            void setBeamCenterX(double);
            void getBeamCenterX(double&);
            void setBeamCenterY(double);
            void getBeamCenterY(double&);
            void setDetectorDistance(double);
            void getDetectorDistance(double&);            
            void setChiIncrement(double);
            void getChiIncrement(double&);
            void setChiStart(double);
            void getChiStart(double&);
            void setKappaIncrement(double);
            void getKappaIncrement(double&);
            void setKappaStart(double);
            void getKappaStart(double&);
            void setOmegaIncrement(double);
            void getOmegaIncrement(double&);
            void setOmegaStart(double);
            void getOmegaStart(double&);
            void setPhiIncrement(double);
            void getPhiIncrement(double&);
            void setPhiStart(double);
            void getPhiStart(double&);
            
			void getCompression(bool&);
   			void setCompression(bool);
			void getCompressionType(CompressionType&) const;
			void setCompressionType(CompressionType);
			void getSerieId(int&);
			void deleteMemoryFiles();
			void disarm();
            void statusUpdate();

			const std::string& getDetectorIp() const;
            const std::string& getTimestampType() const;
            void  setTimestampType(const std::string&);
            void  setCurlDelayMs(double);
		    void setNbFramesPerTriggerIsMaster(bool);
	            
            void getDetectorReadoutTime(double&);
            void setRoiMode(const std::string&);
            void getRoiMode(std::string&);

            void setNbTriggers(int nb_triggers);
            void getNbTriggers(int& nb_triggers);
            void setNbFramesPerTrigger(int nb_frames_per_trigger);
            void getNbFramesPerTrigger(int& nb_frames_per_trigger);

		private:
			enum InternalStatus {IDLE,RUNNING,ERROR};
			class AcqCallback;
			friend class AcqCallback;
			class InitCallback;
			friend class InitCallback;
			void initialiseController(); /// Used during plug-in initialization
			void _acquisition_finished(bool);

            //-----------------------------------------------------------------------------
			//- lima stuff
			int                       m_nb_frames;
			int                       m_image_number;
            int                       m_nb_triggers;
            int                       m_nb_frames_per_trigger;
			double                    m_latency_time;
			TrigMode                  m_trig_mode;

			//- camera stuff
			std::string               m_detector_model;
			std::string               m_detector_type;
			unsigned int              m_maxImageWidth, m_maxImageHeight;
            ImageType                 m_detectorImageType;  
            std::string               m_software_version;

            InternalStatus            m_initilize_state;
			InternalStatus            m_trigger_state;
			int                       m_serie_id;

            //- EigerAPI stuff
			eigerapi::Requests*	      m_requests;
         
			double                    m_temperature;
			double                    m_humidity;
			double                    m_exp_time;
			double                    m_readout_time;
			double                    m_x_pixelsize, m_y_pixelsize;
			Cond                      m_cond;
			std::string               m_detector_ip;
            std::string               m_timestamp_type;
			double                    m_min_frame_time;
            double                    m_min_photon_energy;
            double                    m_max_photon_energy;
            double                    m_min_threshold_energy;
            double                    m_max_threshold_energy;
            
			bool 		              m_nb_frames_per_trigger_is_master;
			
	};
	} // namespace Eiger
} // namespace lima


#endif // EIGERCAMERA_H
