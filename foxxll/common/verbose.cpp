/***************************************************************************
 *  foxxll/common/verbose.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cmath>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <thread>

#include <tlx/die.hpp>

#include <foxxll/common/timer.hpp>
#include <foxxll/msvc_compatibility.hpp>
#include <foxxll/verbose.hpp>

namespace foxxll {

static const double program_start_time_stamp = timestamp();

void print_msg(const char* label, const std::string& msg, unsigned flags)
{
    die("wont be used in future");
}

} // namespace foxxll

// vim: et:ts=4:sw=4
