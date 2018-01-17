/***************************************************************************
 *  foxxll/common/aligned_alloc.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2018 Manuel Penschuck <foxxll@manuel.jetzt>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_COMMON_DIE_WITH_MESSAGE_HEADER
#define FOXXLL_COMMON_DIE_WITH_MESSAGE_HEADER

#include <tlx/die.hpp>

// These macros are a compat layer until we have stream support in die_if/die_unless

//! Check condition X and die miserably if false. Same as die_if()
//! except user additionally pass message
#define die_with_message_if(X, msg)                                         \
    do {                                                                    \
        if ((X)) {                                                          \
            die_with_sstream(                                               \
                "DIE: Assertion \"" #X "\" succeeded!\n " << msg << "\n");  \
        }                                                                   \
    } while (false)


//! Check condition X and die miserably if false. Same as die_unless()
//! except user additionally pass message
#define die_with_message_unless(X, msg)                                   \
    do {                                                                  \
        if (!(X)) {                                                       \
            die_with_sstream(                                             \
                "DIE: Assertion \"" #X "\" failed!\n " << msg << "\n");   \
        }                                                                 \
    } while (false)


#endif // FOXXLL_COMMON_DIE_WITH_MESSAGE_HEADER
