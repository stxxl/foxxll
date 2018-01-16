/***************************************************************************
 *  tests/mng/test_async_schedule.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cstdlib>
#include <random>

#include <foxxll/mng/async_schedule.hpp>
#include <foxxll/verbose.hpp>

// Test async schedule algorithm

int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        STXXL_ERRMSG("Usage: " << argv[0] << " D L m seed");
        return -1;
    }
    const size_t D = strtoul(argv[1], nullptr, 0);
    const size_t L = strtoul(argv[2], nullptr, 0);
    const size_t m = strtoul(argv[3], nullptr, 0);
    uint32_t seed = strtoul(argv[4], nullptr, 0);
    size_t* disks = new size_t[L];
    size_t* prefetch_order = new size_t[L];
    size_t* count = new size_t[D];

    for (size_t i = 0; i < D; i++)
        count[i] = 0;

    std::default_random_engine rnd(seed);
    for (size_t i = 0; i < L; i++)
    {
        disks[i] = rnd() % D;
        count[disks[i]]++;
    }

    for (size_t i = 0; i < D; i++)
        std::cout << "Disk " << i << " has " << count[i] << " blocks" << std::endl;

    foxxll::compute_prefetch_schedule(disks, disks + L, prefetch_order, m, D);

    STXXL_MSG("Prefetch order:");
    for (size_t i = 0; i < L; ++i) {
        STXXL_MSG("request " << prefetch_order[i] << "  on disk " << disks[prefetch_order[i]]);
    }
    STXXL_MSG("Request order:");
    for (size_t i = 0; i < L; ++i) {
        size_t j;
        for (j = 0; prefetch_order[j] != i; ++j) ;
        STXXL_MSG("request " << i << "  on disk " << disks[i] << "  scheduled as " << j);
    }

    delete[] count;
    delete[] disks;
    delete[] prefetch_order;
}

// vim: et:ts=4:sw=4
