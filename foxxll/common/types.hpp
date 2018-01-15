/***************************************************************************
 *  foxxll/common/types.hpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2007 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_COMMON_TYPES_HEADER
#define FOXXLL_COMMON_TYPES_HEADER

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <foxxll/config.hpp>

namespace foxxll {

using external_size_type = uint64_t;       // may require external memory
using external_diff_type = int64_t;        // may require external memory

//! Return the given value casted to the corresponding unsigned type
template <typename Integral>
typename std::make_unsigned<Integral>::type as_unsigned(Integral value)
{
    return static_cast<typename std::make_unsigned<Integral>::type>(value);
}

//! Return the given value casted to the corresponding signed type
template <typename Integral>
typename std::make_signed<Integral>::type as_signed(Integral value)
{
    return static_cast<typename std::make_signed<Integral>::type>(value);
}

} // namespace foxxll

#endif // !FOXXLL_COMMON_TYPES_HEADER
