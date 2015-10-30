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
                                 const std::string& strAPIVersion   ///< [in] Eiger API version string
                                 )
{
    m_ip_addr     = strIP;
    m_api_version = strAPIVersion;

    // Detector Commands
    m_resources["initialize"]    = new ResourceDescription(RESOURCE_COMMAND, "initialize");
    m_resources["arm"]           = new ResourceDescription(RESOURCE_COMMAND, "arm");
    m_resources["disarm"]        = new ResourceDescription(RESOURCE_COMMAND, "disarm");
    m_resources["abort"]         = new ResourceDescription(RESOURCE_COMMAND, "abort");
    m_resources["cancel"]        = new ResourceDescription(RESOURCE_COMMAND, "cancel");
    m_resources["trigger"]       = new ResourceDescription(RESOURCE_COMMAND, "trigger");
    m_resources["status_update"] = new ResourceDescription(RESOURCE_COMMAND, "status_update");

    // Detector Read only values   
    m_resources["temp"]        = new ResourceDescription(RESOURCE_VALUE_RO, 
                                                         "th0_temp", 
                                                         STR_SUBSYSTEM_DETECTOR,
                                                         std::string(STR_EIGER_STATUS)+std::string("/board_000"));

    m_resources["humidity"]    = new ResourceDescription(RESOURCE_VALUE_RO,
                                                         "th0_humidity",
                                                         STR_SUBSYSTEM_DETECTOR,
                                                         std::string(STR_EIGER_STATUS)+std::string("/board_000"));

    m_resources["detector_status"] = new ResourceDescription(RESOURCE_VALUE_RO,
                                                             "state",
                                                             STR_SUBSYSTEM_DETECTOR,
                                                             STR_EIGER_STATUS);

    m_resources["pixeldepth"]   = new ResourceDescription(RESOURCE_VALUE_RO, "bit_depth_readout");
    m_resources["x_pixel_size"] = new ResourceDescription(RESOURCE_VALUE_RO, "x_pixel_size");
    m_resources["y_pixel_size"] = new ResourceDescription(RESOURCE_VALUE_RO, "y_pixel_size");

    m_resources["detector_witdh"]  = new ResourceDescription(RESOURCE_VALUE_RO, "x_pixels_in_detector");
    m_resources["detector_height"] = new ResourceDescription(RESOURCE_VALUE_RO, "y_pixels_in_detector");

    m_resources["description"]  = new ResourceDescription(RESOURCE_VALUE_RO, "description");
    m_resources["detector_number"] = new ResourceDescription(RESOURCE_VALUE_RO, "detector_number");
    m_resources["detector_readout_time"] = new ResourceDescription(RESOURCE_VALUE_RO, "detector_readout_time");

    // Detector Read/Write settings
    m_resources["exposure"]                 = new ResourceDescription(RESOURCE_VALUE_RW, "count_time");
    m_resources["frame_time"]               = new ResourceDescription(RESOURCE_VALUE_RW, "frame_time");
    m_resources["trigger_mode"]             = new ResourceDescription(RESOURCE_VALUE_RW, "trigger_mode");
    m_resources["countrate_correction"]     = new ResourceDescription(RESOURCE_VALUE_RW, "countrate_correction_applied");
    m_resources["flatfield_correction"]     = new ResourceDescription(RESOURCE_VALUE_RW, "flatfield_correction_applied");
    m_resources["efficiency_correction"]    = new ResourceDescription(RESOURCE_VALUE_RW, "efficiency_correction_applied");
    m_resources["pixel_mask"]               = new ResourceDescription(RESOURCE_VALUE_RW, "pixel_mask_applied");
    m_resources["threshold_energy"]         = new ResourceDescription(RESOURCE_VALUE_RW, "threshold_energy");
    m_resources["virtual_pixel_correction"] = new ResourceDescription(RESOURCE_VALUE_RW, "virtual_pixel_correction_applied");
    m_resources["photon_energy"]            = new ResourceDescription(RESOURCE_VALUE_RW, "photon_energy");
    m_resources["nimages"]                  = new ResourceDescription(RESOURCE_VALUE_RW, "nimages");
    m_resources["auto_summation"]           = new ResourceDescription(RESOURCE_VALUE_RW, "auto_summation");


    // Filewriter settings
    m_resources["compression"]       = new ResourceDescription(RESOURCE_VALUE_RW, "compression_enabled", STR_SUBSYSTEM_FILE_WRITER);
    m_resources["name_pattern"]      = new ResourceDescription(RESOURCE_VALUE_RW, "name_pattern", STR_SUBSYSTEM_FILE_WRITER);
    m_resources["nimages_per_file"]  = new ResourceDescription(RESOURCE_VALUE_RW, "nimages_per_file", STR_SUBSYSTEM_FILE_WRITER);
    m_resources["filewriter_status"] = new ResourceDescription(RESOURCE_VALUE_RW, "state", STR_SUBSYSTEM_FILE_WRITER, STR_EIGER_STATUS);

    // File management   
    m_resources["datafile"] = new ResourceDescription(RESOURCE_FILE, std::string("lima_data_000001.h5") ); // "lima_data_000001.h5"

    // File management (TANGODEVIC-1256)
    m_resources["masterfile"] = new ResourceDescription(RESOURCE_FILE, std::string("lima_master.h5") ); // "lima_master.h5"   
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
/// setFileNamePattern
//---------------------------------------------------------------------------
void ResourceFactory::setFileNamePattern(const std::string& pattern)
{
    
    std::map <std::string, ResourceDescription*>::iterator it;
    // data File management       
    it = m_resources.find("datafile");        
    if (it != m_resources.end())
    {
        // found it - delete it
        delete it->second;
        m_resources.erase(it);
    }
    else
        throw EigerException(RESOURCE_NOT_FOUND, "datafile", "ResourceFactory::setFileNamePattern");    
    m_resources["datafile"] = new ResourceDescription(RESOURCE_FILE, pattern+std::string("_data_000001.h5") ); // "%pattern%_data_000001.h5"
    
    // master File management           
    it = m_resources.find("masterfile");    
    if (it != m_resources.end())
    {
        // found it - delete it
        delete it->second;
        m_resources.erase(it);
    }
    else
        throw EigerException(RESOURCE_NOT_FOUND, "masterfile", "ResourceFactory::setFileNamePattern");            
    m_resources["masterfile"] = new ResourceDescription(RESOURCE_FILE, pattern+std::string("_master.h5") ); // "%pattern%_master.h5"    
        
    //send to the detector
    std::shared_ptr<ResourceValue> name_pattern (dynamic_cast<ResourceValue*> (getResource("name_pattern")));
    name_pattern->set(pattern);
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
        std::string url = std::string(HTTP_ROOT) + m_ip_addr + std::string("/");
        std::string api = std::string("/") + std::string(STR_EIGER_API) + std::string("/") + m_api_version + std::string("/");

        ResourceDescription* pDescription = iterFind->second;

        url = url + pDescription->m_subsystem + api + pDescription->m_location + std::string("/") + pDescription->m_name;

        // Create the appropriate descendant depending on eResourceType
        switch (pDescription->m_resource_type)
        {
            case RESOURCE_VALUE_RO:
            case RESOURCE_VALUE_RW:
            {
                pResource = new ResourceValue(url, RESOURCE_VALUE_RW==pDescription->m_resource_type);
            }
                break;


            case RESOURCE_COMMAND:
            {
                pResource = new ResourceCommand(url);
            }
                break;

            case RESOURCE_FILE:
            {
                url = std::string(HTTP_ROOT) + m_ip_addr + std::string("/") + pDescription->m_location
                + std::string("/") + pDescription->m_name;
                pResource = new ResourceFile(url, pDescription->m_name);
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
