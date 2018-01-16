/***************************************************************************
 *  foxxll/io/create_file.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_IO_CREATE_FILE_HEADER
#define FOXXLL_IO_CREATE_FILE_HEADER

#include <string>

#include <foxxll/io/file.hpp>

namespace foxxll {

//! create fileio object from io_impl string and a few parameters
file_ptr create_file(const std::string& io_impl,
                     const std::string& filename,
                     int options,
                     int physical_device_id = file::DEFAULT_QUEUE,
                     int disk_allocator_id = file::NO_ALLOCATOR);

// prototype
class disk_config;

//! create fileio object from disk_config parameter
file_ptr create_file(disk_config& config, int mode,
                     int disk_allocator_id = file::NO_ALLOCATOR);

} // namespace foxxll

#endif // !FOXXLL_IO_CREATE_FILE_HEADER
// vim: et:ts=4:sw=4
