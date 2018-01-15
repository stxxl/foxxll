/***************************************************************************
 *  foxxll/io.hpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_IO_HEADER
#define FOXXLL_IO_HEADER

#include <foxxll/common/aligned_alloc.hpp>
#include <foxxll/io/create_file.hpp>
#include <foxxll/io/disk_queues.hpp>
#include <foxxll/io/file.hpp>
#include <foxxll/io/fileperblock_file.hpp>
#include <foxxll/io/iostats.hpp>
#include <foxxll/io/linuxaio_file.hpp>
#include <foxxll/io/memory_file.hpp>
#include <foxxll/io/mmap_file.hpp>
#include <foxxll/io/request.hpp>
#include <foxxll/io/request_operations.hpp>
#include <foxxll/io/syscall_file.hpp>
#include <foxxll/io/wincall_file.hpp>

//! \c STXXL library namespace
namespace foxxll {

// ...

} // namespace foxxll

#endif // !FOXXLL_IO_HEADER
