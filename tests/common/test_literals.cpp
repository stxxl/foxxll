/***************************************************************************
 *  tests/common/test_literals.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2018 Manuel Penschuck <foxxll@manuel.jetzt>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>

#include <foxxll/common/literals.hpp>

static_assert(1_eB  == 1llu, "");
static_assert(1_ekB == 1000llu, "");
static_assert(1_eMB == 1000llu * 1000llu, "");
static_assert(1_eGB == 1000llu * 1000llu * 1000llu, "");
static_assert(1_eTB == 1000llu * 1000llu * 1000llu * 1000llu, "");
static_assert(1_ePB == 1000llu * 1000llu * 1000llu * 1000llu * 1000llu, "");
static_assert(1_eB  == 1llu, "");
static_assert(1_eKiB == 1024llu, "");
static_assert(1_eMiB == 1024llu * 1024llu, "");
static_assert(1_eGiB == 1024llu * 1024llu * 1024llu, "");
static_assert(1_eTiB == 1024llu * 1024llu * 1024llu * 1024llu, "");
static_assert(1_ePiB == 1024llu * 1024llu * 1024llu * 1024llu * 1024llu, "");

int main() {
    std::cout << "This is a compile-time only test." << std::endl;
    return 0;
}
