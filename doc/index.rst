DECTRIS EIGER 1M Detector System
================================

.. image:: EIGER_1M_432x206.jpg

Introduction
------------
The EIGER 1M is a high performance X-Ray detector system.
The API consists in communicating with the system via its integrated HTTP server using JSON data objects.
A C++ wrapper for LImA is currently being developed at Synchrotron SOLEIL.

Prerequisite
------------

* unknown at the moment

Initialization and Capabilities
-------------------------------
In order to help people understand how the camera plugin has been implemented in LImA this section provides some important information about the developer's choices.

Camera initialization
---------------------
* unknown at the moment

Std capabilites
---------------

This plugin has been implemented in respect of the mandatory capabilites but with some limitations according to some programmer's choices.
We only provide here extra information for a better understanding of the capabilities of the Orca camera.

* HwDetInfo

 * Maximum image size is : 1030 * 1065
 * 12 bit data type is used a the maximum frame rate. (higher bit depth at lower frame rates)
 * Pixel size: 75µm * 75µm
 * Maximum frame rate is 3000Hz
 
* HwSync
	Supported trigger types are:
	
 * unknown at the moment
 

Optional capabilities
---------------------

* HwBin
	Possible binning values are:
 * unknown at the moment

* HwRoi

 * unknown at the moment

* HwShutter

 * unknown at the moment

* Cooling

 * The detector uses liquid cooling.
 * It is unknown at the moment whether the API allows accessing the sensor cooler.
 
Configuration
-------------

How to use
----------
