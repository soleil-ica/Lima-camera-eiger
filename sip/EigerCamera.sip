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
namespace Eiger
{
  class Camera
  {
%TypeHeaderCode
#include <EigerCamera.h>
%End
  public:

    enum Status { Ready, Initialising, Exposure, Readout, Fault };

    Camera(const std::string& detector_ip);
    ~Camera();

    void initialize();

    void startAcq();
    void stopAcq();
    void prepareAcq();

    // -- detector info object
    void getImageType(ImageType& type /Out/);
    void setImageType(ImageType type);

    void getDetectorType(std::string& type /Out/);
    void getDetectorModel(std::string& model /Out/);
    void getDetectorImageSize(Size& size /Out/);
    void getDetectorMaxImageSize(Size& size /Out/);

    //-- Synch control object
    bool checkTrigMode(TrigMode trig_mode);
    void setTrigMode(TrigMode  mode);
    void getTrigMode(TrigMode& mode /Out/);

    void setExpTime(double  exp_time);
    void getExpTime(double& exp_time /Out/);

    void setLatTime(double  lat_time);
    void getLatTime(double& lat_time /Out/);

    void getExposureTimeRange(double& min_expo /Out/, double& max_expo /Out/) const;
    void getLatTimeRange(double& min_lat /Out/, double& max_lat /Out/) const;

    void setNbFrames(int  nb_frames);
    void getNbFrames(int& nb_frames /Out/);
    void getNbHwAcquiredFrames(int &nb_acq_frames /Out/);

    bool isBinningAvailable();

    void getPixelSize(double& sizex /Out/, double& sizey /Out/);

    Status getStatus();
    std::string getCamStatus();
    //			void reset();

    // -- Eiger specific
    void getTemperature(double& /Out/);
    void getHumidity(double& /Out/);

    void setCountrateCorrection(const bool);
    void getCountrateCorrection(bool& /Out/);
    void setFlatfieldCorrection(const bool);
    void getFlatfieldCorrection(bool& /Out/);
    void setAutoSummation(bool);
    void getAutoSummation(bool& /Out/);
    void setEfficiencyCorrection(const bool);
    void getEfficiencyCorrection(bool& value /Out/);
    void setPixelMask(const bool);
    void getPixelMask(bool& /Out/);
    void setThresholdEnergy(const double);
    void getThresholdEnergy(double& /Out/);
    void setVirtualPixelCorrection(const bool);
    void getVirtualPixelCorrection(bool& /Out/);
    void setPhotonEnergy(const double);
    void getPhotonEnergy(double& /Out/);			

    void getCompression(bool& /Out/);
    void setCompression(const bool);
    
    void getSerieId(int& /Out/);
    void deleteMemoryFiles();
    void disarm();
 };
};
