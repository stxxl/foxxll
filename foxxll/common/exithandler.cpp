/***************************************************************************
 *  foxxll/common/exithandler.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cstdlib>

#include <foxxll/common/exithandler.hpp>

// 1. do nothing for default handler
// 2. #define FOXXLL_NON_DEFAULT_EXIT_HANDLER for a handler that does not use atexit()
// 3. #define FOXXLL_EXTERNAL_EXIT_HANDLER to provide your own implementation

#ifdef STXXL_EXTERNAL_EXIT_HANDLER
static_assert(false, "STXXL_EXTERNAL_EXIT_HANDLER was renamed to FOXXLL_EXTERNAL_EXIT_HANDLER");
#endif

#ifdef STXXL_NON_DEFAULT_EXIT_HANDLER
static_assert(false, "STXXL_NON_DEFAULT_EXIT_HANDLER was renamed to FOXXLL_NON_DEFAULT_EXIT_HANDLER");
#endif

#ifndef FOXXLL_EXTERNAL_EXIT_HANDLER
#ifndef FOXXLL_NON_DEFAULT_EXIT_HANDLER

namespace foxxll {

// default exit handler
int register_exit_handler(void (* function)(void))
{
    return atexit(function);
}

// default exit handler
void run_exit_handlers()
{
    // nothing to do
}

} // namespace foxxll

#else // FOXXLL_NON_DEFAULT_EXIT_HANDLER

#include <mutex>
#include <vector>

namespace foxxll {

std::mutex exit_handler_mutex;
std::vector<void (*)(void)> exit_handlers;

int register_exit_handler(void (* function)(void))
{
    std::unique_lock<std::mutex> lock(exit_handler_mutex);
    exit_handlers.push_back(function);
    return 0;
}

// default exit handler
void run_exit_handlers()
{
    std::unique_lock<std::mutex> lock(exit_handler_mutex);
    while (!exit_handlers.empty()) {
        (*(exit_handlers.back()))();
        exit_handlers.pop_back();
    }
}

} // namespace foxxll

#endif // FOXXLL_NON_DEFAULT_EXIT_HANDLER
#endif // FOXXLL_EXTERNAL_EXIT_HANDLER
