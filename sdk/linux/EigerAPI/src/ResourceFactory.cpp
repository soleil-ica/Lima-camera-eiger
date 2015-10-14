//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : ResourceFactory.cpp
/// Creation : 2014/07/22
/// Author   : William BOULADOUX
///
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the GNU General Public License as published by the Free Software
/// Foundation; version 2 of the License.
/// 
/// This program is distributed in the hope that it will be useful, but WITHOUT 
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
/// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
///
//*****************************************************************************

#include "EigerDefines.h"
#include "ResourceFactory.h"
#include "ResourceDescription.h"
#include "ResourceValue.h"
#include "ResourceCommand.h"
#include "ResourceFile.h"
#include <iostream>

namespace eigerapi
{

//---------------------------------------------------------------------------
/// Constructor
//---------------------------------------------------------------------------
ResourceFactory::ResourceFactory(const std::string& strIP,           ///< [in] detector server IP
                                 const std::string& strAPIVersion)   ///< [in] Eiger API version string
{
   m_ipaddr     = strIP;
   m_APIVersion = strAPIVersion;

// Detector Commands
   m_resources["initialize"]    = new ResourceDescription(ERESOURCETYPE_COMMAND, "initialize");
   m_resources["arm"]           = new ResourceDescription(ERESOURCETYPE_COMMAND, "arm"); 
   m_resources["disarm"]        = new ResourceDescription(ERESOURCETYPE_COMMAND, "disarm");
   m_resources["trigger"]       = new ResourceDescription(ERESOURCETYPE_COMMAND, "trigger");
   m_resources["status_update"] = new ResourceDescription(ERESOURCETYPE_COMMAND, "status_update");
   
// Detector Read only values   
   m_resources["temp"]        = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, 
                                                        "th0_temp", 
                                                        CSTR_SUBSYSTEMDETECTOR,
                                                        std::string(CSTR_EIGERSTATUS)+std::string("/board_000"));
   
   m_resources["humidity"]    = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, 
                                                        "th0_humidity", 
                                                        CSTR_SUBSYSTEMDETECTOR,
                                                        std::string(CSTR_EIGERSTATUS)+std::string("/board_000"));
                                                        
   m_resources["detector_status"] = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, 
                                                            "state", 
                                                            CSTR_SUBSYSTEMDETECTOR,
                                                            CSTR_EIGERSTATUS);

   m_resources["pixeldepth"]   = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, "bit_depth_readout");
   m_resources["x_pixel_size"] = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, "x_pixel_size");
   m_resources["y_pixel_size"] = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, "y_pixel_size");

   m_resources["detector_witdh"]  = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, "x_pixels_in_detector");
   m_resources["detector_height"] = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, "y_pixels_in_detector");

   m_resources["description"]  = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, "description");
   m_resources["detector_number"] = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, "detector_number");
   m_resources["detector_readout_time"] = new ResourceDescription(ERESOURCETYPE_VALUE_READONLY, "detector_readout_time");
    
// Detector Read/Write settings
   m_resources["exposure"]                 = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "count_time");
   m_resources["frame_time"]               = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "frame_time");
   m_resources["trigger_mode"]             = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "trigger_mode");
   m_resources["countrate_correction"]     = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "countrate_correction_applied");
   m_resources["flatfield_correction"]     = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "flatfield_correction_applied");
   m_resources["efficiency_correction"]    = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "efficiency_correction_applied");
   m_resources["pixel_mask"]               = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "pixel_mask_applied");
   m_resources["threshold_energy"]         = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "threshold_energy");
   m_resources["virtual_pixel_correction"] = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "virtual_pixel_correction_applied");
   m_resources["photon_energy"]            = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "photon_energy");
   m_resources["nimages"]                  = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "nimages");
   m_resources["auto_summation"]           = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "auto_summation");


// Filewriter settings
   m_resources["compression"]       = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "compression_enabled", CSTR_SUBSYSTEMFILEWRITER);
   m_resources["name_pattern"]      = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "name_pattern", CSTR_SUBSYSTEMFILEWRITER);
   m_resources["nimages_per_file"]  = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "nimages_per_file", CSTR_SUBSYSTEMFILEWRITER);
   m_resources["filewriter_status"] = new ResourceDescription(ERESOURCETYPE_VALUE_READWRITE, "state", CSTR_SUBSYSTEMFILEWRITER, CSTR_EIGERSTATUS);
   
// File management
   m_resources["datafile"] = new ResourceDescription(ERESOURCETYPE_FILE, CSTR_EIGERDATAFILE ); // "lima_data_000000.h5"
   
// File management (TANGODEVIC-1256)
   m_resources["masterfile"] = new ResourceDescription(ERESOURCETYPE_FILE, CSTR_EIGERMASTERFILE ); // "lima_master.h5"   
}


//---------------------------------------------------------------------------
/// Destructor
//---------------------------------------------------------------------------
ResourceFactory::~ResourceFactory()
{
   std::map <std::string, ResourceDescription*>::iterator iter = m_resources.begin();
   while (m_resources.end()!=iter)
   {
      delete(iter->second);
      ++iter;
   }
   m_resources.clear();
}


//---------------------------------------------------------------------------
/// Create a new resource given its name
/*!
@return a new allocated resource object or NULL if no resource with this name exists
*/
//---------------------------------------------------------------------------
Resource* ResourceFactory::getResource(const std::string& resourceName) ///< [in] name of the resource
{
   Resource* pResource = NULL;
	
	// find the description of the given resource
   tResourceMap::const_iterator iterFind = m_resources.find(resourceName);
   if (m_resources.end()!=iterFind)
   {
      std::string url = std::string(HTTP_ROOT) + m_ipaddr + std::string("/");
      std::string api = std::string("/") + std::string(CSTR_EIGERAPI) + std::string("/") + m_APIVersion + std::string("/");
    
      ResourceDescription* pDescription = iterFind->second;    
      
      url = url + pDescription->m_subsystem + api + pDescription->m_location + std::string("/") + pDescription->m_name;
      
      // Create the appropriate descendant depending on eResourceType
      switch (pDescription->m_eResourceType)
      {
         case ERESOURCETYPE_VALUE_READONLY:
         case ERESOURCETYPE_VALUE_READWRITE:
         {                  
            pResource = new ResourceValue(url, ERESOURCETYPE_VALUE_READWRITE==pDescription->m_eResourceType);
         }
         break;
        
        
         case ERESOURCETYPE_COMMAND:
         {
            pResource = new ResourceCommand(url);
         }
         break;
         
         case ERESOURCETYPE_FILE:
         {
            url = std::string(HTTP_ROOT) + m_ipaddr + std::string("/") + pDescription->m_location 
                                         + std::string("/") + pDescription->m_name;
            pResource = new ResourceFile(url);            
         }
         break;
      }
   }
   else
   {
      throw EigerException(RESOURCE_NOT_FOUND, resourceName.c_str(), "ResourceFactory::getResource");
   }
   
	return pResource;
}


//---------------------------------------------------------------------------
/// Get the list of the available resources
//---------------------------------------------------------------------------
void ResourceFactory::getResourceList(std::list<std::string>& lst) ///< [out] list of the available resources
{
   tResourceMap::const_iterator iter = m_resources.begin();
   while (m_resources.end()!=iter)
   {
      lst.push_back(iter->first);
      ++iter;
   }   
}
}
