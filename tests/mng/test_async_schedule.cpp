/***************************************************************************
 *  tests/mng/test_async_schedule.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <foxxll/mng/async_schedule.hpp>
#include <foxxll/verbose.hpp>

#include <cstdlib>
#include <random>

using int_type = foxxll::int_type;

// Test async schedule algorithm

int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        STXXL_ERRMSG("Usage: " << argv[0] << " D L m seed");
        return -1;
    }
    const int D = atoi(argv[1]);
    const int L = atoi(argv[2]);
    const int_type m = atoi(argv[3]);
    uint32_t seed = atoi(argv[4]);
    int_type* disks = new int_type[L];
    int_type* prefetch_order = new int_type[L];
    int* count = new int[D];

    for (int i = 0; i < D; i++)
        count[i] = 0;

    std::default_random_engine rnd(seed);
    for (int i = 0; i < L; i++)
    {
        disks[i] = rnd() % D;
        count[disks[i]]++;
    }

    for (int i = 0; i < D; i++)
        std::cout << "Disk " << i << " has " << count[i] << " blocks" << std::endl;

    foxxll::compute_prefetch_schedule(disks, disks + L, prefetch_order, m, D);

    STXXL_MSG("Prefetch order:");
    for (int i = 0; i < L; ++i) {
        STXXL_MSG("request " << prefetch_order[i] << "  on disk " << disks[prefetch_order[i]]);
    }
    STXXL_MSG("Request order:");
    for (int i = 0; i < L; ++i) {
        int j;
        for (j = 0; prefetch_order[j] != i; ++j) ;
        STXXL_MSG("request " << i << "  on disk " << disks[i] << "  scheduled as " << j);
    }

    delete[] count;
    delete[] disks;
    delete[] prefetch_order;
}

// vim: et:ts=4:sw=4
