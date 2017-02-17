/***************************************************************************
 *  include/stxxl/bits/parallel.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2011 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2015 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PARALLEL_HEADER
#define STXXL_PARALLEL_HEADER

#include <stxxl/bits/config.h>

#include <cassert>

#if STXXL_PARALLEL
 #include <omp.h>
#endif

#include <stxxl/bits/common/settings.h>
#include <stxxl/bits/verbose.h>

#if defined(_GLIBCXX_PARALLEL)
//use _STXXL_FORCE_SEQUENTIAL to tag calls which are not worthwhile parallelizing
#define _STXXL_FORCE_SEQUENTIAL , __gnu_parallel::sequential_tag()
#else
#define _STXXL_FORCE_SEQUENTIAL
#endif

#if 0
// sorting triggers is done sequentially
#define _STXXL_SORT_TRIGGER_FORCE_SEQUENTIAL _STXXL_FORCE_SEQUENTIAL
#else
// sorting triggers may be parallelized
#define _STXXL_SORT_TRIGGER_FORCE_SEQUENTIAL
#endif

#if !STXXL_PARALLEL
#undef STXXL_PARALLEL_MULTIWAY_MERGE
#define STXXL_PARALLEL_MULTIWAY_MERGE 0
#endif

#if defined(STXXL_PARALLEL_MODE) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) < 40400)
#undef STXXL_PARALLEL_MULTIWAY_MERGE
#define STXXL_PARALLEL_MULTIWAY_MERGE 0
#endif

#if !defined(STXXL_PARALLEL_MULTIWAY_MERGE)
#define STXXL_PARALLEL_MULTIWAY_MERGE 1
#endif

#if !defined(STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD)
#define STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD 0
#endif

#endif // !STXXL_PARALLEL_HEADER
// vim: et:ts=4:sw=4
