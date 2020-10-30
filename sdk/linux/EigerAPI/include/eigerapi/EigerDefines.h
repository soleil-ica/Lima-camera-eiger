//*****************************************************************************
/// Synchrotron SOLEIL
///
/// EigerAPI C++
/// File     : EigerDefines.h
/// Creation : 2014/09/03
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
//
/*!  \brief     Defines for EigerAPI
*/
//*****************************************************************************

#ifndef _EIGERDEFINES_H
#define _EIGERDEFINES_H

#include <iostream>
#include <sstream>
#include <exception>
#include <memory>

namespace eigerapi
{
const char RESOURCE_NOT_FOUND[] = "Resource not found: ";
const char JSON_PARSE_FAILED[] = "Json parse failed: ";
const char DATA_TYPE_NOT_HANDLED[] = "Data type not handled: ";

//=============================================================================
/// Eiger exceptions
///
/// This class is designed to hold Eiger exceptions
//=============================================================================
class EigerException : public std::exception
{
public:
    EigerException(const char *pcszDesc, const char *pcszArg, const char *pcszOrigin,
                   const char *filename = NULL)
    {
        std::ostringstream oss;
        oss << std::endl
            << "--------------------------------------------------------------------------------------------" << std::endl
            << "EigerException " << std::endl
            << "Description: " << pcszDesc << ":" << pcszArg << std::endl
            << "Origin     : " << pcszOrigin << std::endl;
        if (filename)
            oss << "Location   : " << filename << std::endl;

        oss << "--------------------------------------------------------------------------------------------" << std::endl;

        msg = oss.str();
    }

    virtual ~EigerException() throw()
    {
    }

    virtual const char *what() const throw()
    {
        return msg.c_str();
    }

    /// Prints error message on console
    void dump(void) const throw()
    {
        std::cerr << what() << "\n";
    }

private:
    std::string msg;
};

} // namespace eigerapi

#define EIGER_EXPEND_LINE__SUB(x) #x
#define EIGER_EXPEND_LINE_(x) EIGER_EXPEND_LINE__SUB(x)
#define EIGER_LOCATION __FILE__ " line " EIGER_EXPEND_LINE_(__LINE__)

#define THROW_EIGER_EXCEPTION(pcszDesc, pcszArg) \
    throw eigerapi::EigerException(pcszDesc, pcszArg, __PRETTY_FUNCTION__, EIGER_LOCATION)

#endif
