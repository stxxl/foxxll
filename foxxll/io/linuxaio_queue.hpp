/***************************************************************************
 *  foxxll/io/linuxaio_queue.hpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2011 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *  Copyright (C) 2018 Manuel Penschuck <foxxll@manuel.jetzt>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_IO_LINUXAIO_QUEUE_HEADER
#define FOXXLL_IO_LINUXAIO_QUEUE_HEADER

#include <foxxll/io/linuxaio_file.hpp>

#if STXXL_HAVE_LINUXAIO_FILE

#include <atomic>
#include <list>
#include <mutex>

#include <foxxll/io/request_queue_impl_worker.hpp>

#include <linux/aio_abi.h>

#define FOXXLL_LINUXAIO_QUEUE_STATS 1

#include <foxxll/io/request_queue_impl_worker.hpp>

namespace foxxll {

//! \addtogroup reqlayer
//! \{

//! Queue for linuxaio_file(s)
//!
//! Only one queue exists in a program, i.e. it is a singleton.
class linuxaio_queue : public request_queue_impl_worker
{
    friend class linuxaio_request;

    using self_type = linuxaio_queue;

private:
    //! \defgroup OS related
    //! \{

    //! OS context
    aio_context_t context_;

    //! used by our friend linuxaio_request
    aio_context_t get_io_context() { return context_; }

    //! max number of OS requests
    const size_t os_queue_length_;

    //! Negotiates with OS the queue length and sets up the context
    //! \return length of OS's request queue
    size_t setup_context(size_t requeted);

    //! \}

private:
    //! \defgroup Queue Handling
    //! \{

    //! storing linuxaio_request* would drop ownership
    using queue_type = std::list<request_ptr>;

    // "waiting" request have been submitted to this queue, but not yet to the OS
    std::mutex waiting_mtx_;
    queue_type waiting_requests_;

    // "posted" requests have been submitted to the OS and may already be completed
    std::mutex posted_mtx_;
    queue_type posted_requests_;

    // In the rare event that two requests added in fast succession target the
    // same EM data, we delay the second request to avoid reordering by the kernel.
    queue_type delayed_requests_;

    //! Number of requests posted to the kernel and not yet completed
    std::atomic<int64_t> no_requests_posted_ { 0 };

    //! \}

private:
    //! \defgroup Workers and thread handling
    //! \{

    // The posting thread submits waiting requests to the OS (and may handle
    // the completion of requests)
    std::thread post_thread_;
    shared_state<thread_state> post_thread_state_ { NOT_RUNNING };
    std::condition_variable post_thread_cv_;
    void post_requests();

    // The wait thread uses syscall(getevents) to handle the completion of requests
    std::thread wait_thread_;
    shared_state<thread_state> wait_thread_state_ { NOT_RUNNING };
    std::condition_variable wait_thread_cv_;
    void wait_requests();

    void handle_events(io_event* events, long num_events, bool canceled);

    //! \}

private:
    //! \defgroup Statistics
    //! \{

    // Some statistics, in case we want it. This code is rather experimental and
    // it is unclear whether we want it in production builds; for the moment its
    // the easiest solution
#if FOXXLL_LINUXAIO_QUEUE_STATS
    std::atomic<int64_t> stat_requests_added_ { 0 };
    std::atomic<int64_t> stat_requests_delayed_ { 0 };
    std::atomic<int64_t> stat_syscall_submit_ { 0 };
    std::atomic<int64_t> stat_syscall_submit_repeat_ { 0 };
    std::atomic<int64_t> stat_syscall_failed_post_ { 0 };
    std::atomic<int64_t> stat_syscall_getevents_ { 0 };
    std::atomic<int64_t> stat_syscall_cancel_ { 0 };
#endif
    //! \}

public:
    //! Construct queue. Requests max number of requests simultaneously
    //! submitted to disk, 0 means as many as possible
    explicit linuxaio_queue(size_t desired_queue_length = 0);

    void add_request(request_ptr& req) final;
    bool cancel_request(request_ptr& req) final;

    ~linuxaio_queue();
};

//! \}

} // namespace foxxll

#endif // #if STXXL_HAVE_LINUXAIO_FILE

#endif // !FOXXLL_IO_LINUXAIO_QUEUE_HEADER
// vim: et:ts=4:sw=4
