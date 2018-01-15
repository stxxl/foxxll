/***************************************************************************
 *  foxxll/io/request_queue_impl_worker.hpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_REQUEST_QUEUE_IMPL_WORKER_HEADER
#define STXXL_IO_REQUEST_QUEUE_IMPL_WORKER_HEADER

#include <foxxll/common/semaphore.hpp>
#include <foxxll/common/shared_state.hpp>
#include <foxxll/config.hpp>
#include <foxxll/io/request_queue.hpp>

#include <thread>

namespace foxxll {

//! \addtogroup reqlayer
//! \{

//! Implementation of request queue worker threads. Worker threads can be
//! started by start_thread and stopped with stop_thread. The queue state is
//! checked before termination and updated afterwards.
class request_queue_impl_worker : public request_queue
{
protected:
    enum thread_state { NOT_RUNNING, RUNNING, TERMINATING, TERMINATED };

protected:
    void start_thread(
        std::function<void*(void*)> worker, void* arg,
        std::thread& t, shared_state<thread_state>& s);

    void stop_thread_with_callback(
        std::thread& t, shared_state<thread_state>& s, std::function<void()> f);

    void stop_thread(
        std::thread& t, shared_state<thread_state>& s, semaphore& sem);

};

//! \}

} // namespace foxxll

#endif // !STXXL_IO_REQUEST_QUEUE_IMPL_WORKER_HEADER
// vim: et:ts=4:sw=4
