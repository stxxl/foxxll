/***************************************************************************
 *  tests/common/test_uint_types.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <foxxll/common/uint_types.hpp>
#include <foxxll/verbose.hpp>

// forced instantiation
template class foxxll::uint_pair<uint8_t>;
template class foxxll::uint_pair<uint16_t>;

template <typename uint>
void dotest(unsigned int nbytes)
{
    // simple initialize
    uint a = 42;

    // check sizeof (again)
    FOXXLL_CHECK(sizeof(a) == nbytes);

    // count up 1024 and down again
    uint b = 0xFFFFFF00;
    uint b_save = b;

    uint64_t b64 = b;
    for (unsigned int i = 0; i < 1024; ++i)
    {
        FOXXLL_CHECK(b.u64() == b64);
        FOXXLL_CHECK(b.ull() == b64);
        FOXXLL_CHECK(b != a);
        ++b, ++b64;
    }

    FOXXLL_CHECK(b != b_save);

    for (unsigned int i = 0; i < 1024; ++i)
    {
        FOXXLL_CHECK(b.u64() == b64);
        FOXXLL_CHECK(b.ull() == b64);
        FOXXLL_CHECK(b != a);
        --b, --b64;
    }

    FOXXLL_CHECK(b.u64() == b64);
    FOXXLL_CHECK(b.ull() == b64);
    FOXXLL_CHECK(b == b_save);

    // check min and max value
    FOXXLL_CHECK(uint::min() <= a);
    FOXXLL_CHECK(uint::max() >= a);

    FOXXLL_CHECK(std::numeric_limits<uint>::min() < a);
    FOXXLL_CHECK(std::numeric_limits<uint>::max() > a);

    // check simple math
    a = 42;
    a = a + a;
    FOXXLL_CHECK(a == uint(84));
    FOXXLL_CHECK(a.ull() == 84);

    a += uint(0xFFFFFF00);
    FOXXLL_CHECK(a.ull() == 84 + (unsigned long long)(0xFFFFFF00));
}

int main()
{
    dotest<foxxll::uint40>(5);
    dotest<foxxll::uint48>(6);

    return 0;
}
