/***************************************************************************
 *  foxxll/mng/async_schedule.hpp
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

#ifndef FOXXLL_MNG_ASYNC_SCHEDULE_HEADER
#define FOXXLL_MNG_ASYNC_SCHEDULE_HEADER

// Implements the "prudent prefetching" as described in
// D. Hutchinson, P. Sanders, J. S. Vitter: Duality between prefetching
// and queued writing on parallel disks, 2005
// DOI: 10.1137/S0097539703431573

#include <foxxll/common/types.hpp>
#include <tlx/simple_vector.hpp>

namespace foxxll {

void compute_prefetch_schedule(
    const size_t* first,
    const size_t* last,
    size_t* out_first,
    size_t m,
    size_t D);

inline void compute_prefetch_schedule(
    size_t* first,
    size_t* last,
    size_t* out_first,
    size_t m,
    size_t D)
{
    compute_prefetch_schedule(static_cast<const size_t*>(first), static_cast<const size_t*>(last), out_first, m, D);
}

template <typename RunType>
void compute_prefetch_schedule(
    const RunType& input,
    size_t* out_first,
    size_t m,
    size_t D)
{
    const size_t L = input.size();
    tlx::simple_vector<size_t> disks(L);
    for (size_t i = 0; i < L; ++i)
        disks[i] = input[i].bid.storage->get_device_id();
    compute_prefetch_schedule(disks.begin(), disks.end(), out_first, m, D);
}

template <typename BidIteratorType>
void compute_prefetch_schedule(
    BidIteratorType input_begin,
    BidIteratorType input_end,
    size_t* out_first,
    size_t m,
    size_t D)
{
    const size_t L = input_end - input_begin;
    tlx::simple_vector<size_t> disks(L);
    size_t i = 0;
    for (BidIteratorType it = input_begin; it != input_end; ++it, ++i)
        disks[i] = it->storage->get_device_id();
    compute_prefetch_schedule(disks.begin(), disks.end(), out_first, m, D);
}

} // namespace foxxll

#endif // !FOXXLL_MNG_ASYNC_SCHEDULE_HEADER
