/***************************************************************************
 *  foxxll/common/version.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2007, 2008, 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *  Copyright (C) 2018 Manuel Penschuck <foxxll@manuel.jetzt>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <foxxll/config.hpp>
#include <foxxll/version.hpp>

namespace foxxll {

int version_major()
{
    return FOXXLL_VERSION_MAJOR;
}

int version_minor()
{
    return FOXXLL_VERSION_MINOR;
}

int version_patch()
{
    return FOXXLL_VERSION_PATCH;
}

int version_integer()
{
    return FOXXLL_VERSION_INTEGER;
}

std::string get_library_version_string()
{
    return get_version_string();
}

std::string get_library_version_string_long()
{
    return get_version_string_long();
}

} // namespace foxxll

// vim: et:ts=4:sw=4
