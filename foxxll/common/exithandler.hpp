/***************************************************************************
 *  foxxll/common/exithandler.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_COMMON_EXITHANDLER_HEADER
#define FOXXLL_COMMON_EXITHANDLER_HEADER

namespace foxxll {

// There are several possibilities for the exit handlers.  To use the default
// implementation (which uses atexit()), nothing special has to be done.
//
// To work around problems with atexit() being used in a dll you may #define
// STXXL_NON_DEFAULT_EXIT_HANDLER at library compilation time.  In this case
// the library/application should call foxxll::run_exit_handlers() during
// shutdown.
//
// To provide your own exit handler implementation, #define
// STXXL_EXTERNAL_EXIT_HANDLER and implement foxxll::register_exit_handler(void
// (*)(void)) and foxxll::run_exit_handlers() in your application.

int register_exit_handler(void (* function)(void));
void run_exit_handlers();

} // namespace foxxll

#endif // !FOXXLL_COMMON_EXITHANDLER_HEADER
// vim: et:ts=4:sw=4
