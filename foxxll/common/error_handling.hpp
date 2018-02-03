/***************************************************************************
 *  foxxll/common/error_handling.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2007-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_COMMON_ERROR_HANDLING_HEADER
#define FOXXLL_COMMON_ERROR_HANDLING_HEADER

/** \file error_handling.h
 * Macros for convenient error checking and reporting via exception.
 */

#include <cerrno>
#include <cstring>
#include <sstream>

#include <foxxll/common/exceptions.hpp>
#include <foxxll/config.hpp>

namespace foxxll {

#if FOXXLL_MSVC
 #define FOXXLL_PRETTY_FUNCTION_NAME __FUNCTION__
#else
 #define FOXXLL_PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#endif

////////////////////////////////////////////////////////////////////////////

//! Throws exception_type with "Error in [location] : [error_message]"
#define FOXXLL_THROW2(exception_type, location, error_message)    \
    do {                                                          \
        std::ostringstream msg;                                   \
        msg << "Error in " << location << " : " << error_message; \
        throw exception_type(msg.str());                          \
    } while (false)

//! Throws exception_type with "Error in [function] : [error_message]"
#define FOXXLL_THROW(exception_type, error_message) \
    FOXXLL_THROW2(                                  \
        exception_type,                             \
        FOXXLL_PRETTY_FUNCTION_NAME,                \
        error_message                               \
    )

//! Throws exception_type with "Error in [function] : [error_message] : [errno_value message]"
#define FOXXLL_THROW_ERRNO2(exception_type, error_message, errno_value) \
    FOXXLL_THROW2(                                                      \
        exception_type,                                                 \
        FOXXLL_PRETTY_FUNCTION_NAME,                                    \
        error_message << " : " << strerror(errno_value)                 \
    )

//! Throws exception_type with "Error in [function] : [error_message] : [errno message]"
#define FOXXLL_THROW_ERRNO(exception_type, error_message) \
    FOXXLL_THROW_ERRNO2(exception_type, error_message, errno)

//! Throws std::invalid_argument with "Error in [function] : [error_message]"
#define FOXXLL_THROW_INVALID_ARGUMENT(error_message) \
    FOXXLL_THROW2(                                   \
        std::invalid_argument,                       \
        FOXXLL_PRETTY_FUNCTION_NAME,                 \
        error_message                                \
    )

//! Throws exception_type if (expr) with "Error in [function] : [error_message]"
#define FOXXLL_THROW_IF(expr, exception_type, error_message) \
    do {                                                     \
        if (expr) {                                          \
            FOXXLL_THROW(exception_type, error_message);     \
        }                                                    \
    } while (false)

//! Throws exception_type if (expr) with "Error in [function] : [error_message] : [errno message]"
#define FOXXLL_THROW_ERRNO_IF(expr, exception_type, error_message) \
    do {                                                           \
        if (expr) {                                                \
            FOXXLL_THROW_ERRNO(exception_type, error_message);     \
        }                                                          \
    } while (false)

//! Throws exception_type if (expr != 0) with "Error in [function] : [error_message] : [errno message]"
#define FOXXLL_THROW_ERRNO_NE_0(expr, exception_type, error_message) \
    FOXXLL_THROW_ERRNO_IF((expr) != 0, exception_type, error_message)

#if FOXXLL_WINDOWS || defined(__MINGW32__)

//! Throws exception_type with "Error in [function] : [error_message] : [formatted GetLastError()]"
#define FOXXLL_THROW_WIN_LASTERROR(exception_type, error_message)        \
    do {                                                                 \
        LPVOID lpMsgBuf;                                                 \
        DWORD dw = GetLastError();                                       \
        FormatMessage(                                                   \
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, \
            nullptr, dw,                                                 \
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),                   \
            (LPTSTR)&lpMsgBuf,                                           \
            0, nullptr                                                   \
        );                                                               \
        std::ostringstream msg;                                          \
        msg << "Error in " << FOXXLL_PRETTY_FUNCTION_NAME                \
            << " : " << error_message                                    \
            << " : error code " << dw << " : " << ((char*)lpMsgBuf);     \
        LocalFree(lpMsgBuf);                                             \
        throw exception_type(msg.str());                                 \
    } while (false)

#endif

} // namespace foxxll

#endif // !FOXXLL_COMMON_ERROR_HANDLING_HEADER
