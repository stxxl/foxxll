/***************************************************************************
 *  foxxll/defines.hpp
 *
 *  Document all defines that may change the behavior of FOXXLL.
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_DEFINES_HEADER
#define FOXXLL_DEFINES_HEADER

//#define FOXXLL_HAVE_MMAP_FILE 0/1
//#define FOXXLL_HAVE_WINCALL_FILE 0/1
//#define FOXXLL_HAVE_LINUXAIO_FILE 0/1
// default: 0/1 (platform and type dependent)
// used in: io/*_file.h, io/*_file.cpp, mng/mng.cpp
// affects: library
// effect:  enables/disables some file implementations

//#define FOXXLL_CHECK_BLOCK_ALIGNING
// default: not defined
// used in: io/*_file.cpp
// effect:  call request::check_alignment() from request::request(...)

//#define FOXXLL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION 0/1
// default: 1
// used in: io/*_queue*.cpp
// affects: library
// effect:  check (and warn) for multiple concurrently pending I/O requests
//          for the same block, usually causing coherency problems on
//          out-of-order execution

//#define FOXXLL_DO_NOT_COUNT_WAIT_TIME
// default: not defined
// used in: io/iostats.{h,cpp}
// effect:  makes calls to wait time counting functions no-ops

//#define FOXXLL_WAIT_LOG_ENABLED
// default: not defined
// used in: common/log.cpp, io/iostats.cpp
// effect:  writes wait timing information to the file given via environment
//          variable STXXLWAITLOGFILE, does nothing if this is not defined

//#define FOXXLL_CHECK_ORDER_IN_SORTS 0/1
// default: 0
// used in: algo/*sort.h, stream/sort_stream.h, containers/priority_queue.h
// effect if set to 1: perform additional checking of sorted results

//#define FOXXLL_HACK_SINGLE_IO_THREAD
// default: not defined
// used in: io/disk_queues.h
// affects: programs
// effect if defined: uses only a single I/O thread instead of one per disk
//          used e.g. by EcoSort which puts input file, output file and
//          scratch on a single disk (RAID0)

#endif // !FOXXLL_DEFINES_HEADER
