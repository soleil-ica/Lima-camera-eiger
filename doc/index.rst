DECTRIS EIGER 1M Detector System
================================

.. image:: EIGER_1M_432x206.jpg

Introduction
------------
| The EIGER 1M is a high performance X-Ray detector system.
| It is made of two subsystems: a detector and a control server.
| The control server is driven using an HTTP RESTful interface.
| A C++ API for LImA has been developed at Synchrotron SOLEIL.

Prerequisite
------------
* This is a Linux plug-in.

Initialization and Capabilities
-------------------------------
In order to help people understand how the camera plug-in has been implemented in LImA this section provides some important information about the developer's choices.

Camera initialization
---------------------
Initialization is performed automatically when the plug-in starts (the control server must be reachable on the network).

Std capabilities
----------------

* HwDetInfo
+------------------------+-----------------------+
| Capability             | Value                 |
+========================+=======================+
| Maximum image size     | 1030 * 1065           | 
+------------------------+-----------------------+
| Pixel depth            | 12 bits               |
+------------------------+-----------------------+
| Pixel size             | 75µm * 75µm           |
+------------------------+-----------------------+
| Maximum frame rate     | 3000Hz                |
+------------------------+-----------------------+

* HwSync
	Supported trigger types are:
 * IntTrig
 * ExtTrigSingle
 * ExtTrigMult
 * ExtGate
 
* There is no hardware support for binning or roi.
* There is no shutter control.

Optional capabilities
---------------------

* Cooling

 * The detector uses liquid cooling.
 * The API allows accessing the temperature and humidity as read-only values.

| At the moment, the specific device supports the control of the following features of the Eiger Dectris API.
| (Extended description can be found in the Eiger API user manual from Dectris).

* Accumulation mode
* LZ4 Compression
* Countrate correction
* Efficiency correction
* Flatfield correction
* Photon energy
* Threshold energy
* Pixelmask
* Virtual pixel correction

Configuration
-------------

* Device configuration
The default values of the following properties must be updated in the specific device to meet your system configuration.

+------------------------+---------------------------------------------------------------------------------------------------+----------------+
| Property name          | Description                                                                                       | Default value  | 
+========================+===================================================================================================+================+
| DetectorIP             | Defines the IP address of the Eiger control server (ex: 192.168.10.1)                             |      127.0.0.1 |
+------------------------+---------------------------------------------------------------------------------------------------+----------------+
| TargetPath             | A path having file creation and write access (ex: /tmp). It will be used to store temporary files.|           /tmp |
+------------------------+---------------------------------------------------------------------------------------------------+----------------+
