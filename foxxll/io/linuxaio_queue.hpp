/***************************************************************************
 *  foxxll/io/linuxaio_queue.hpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2011 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_LINUXAIO_QUEUE_HEADER
#define STXXL_IO_LINUXAIO_QUEUE_HEADER

#include <foxxll/io/linuxaio_file.hpp>

#if STXXL_HAVE_LINUXAIO_FILE

#include <tlx/ring_buffer.hpp>
#include <foxxll/io/request_target.hpp>
#include <foxxll/io/request_queue_impl_worker.hpp>

#include <linux/aio_abi.h>

#include <atomic>
#include <list>
#include <mutex>

#define FOXXLL_LINUXAIO_QUEUE_STATS 1

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
    //! OS context_
    aio_context_t context_;

    //! storing linuxaio_request* would drop ownership
    using queue_type = std::list<request_ptr>;

    // "waiting" request have submitted to this queue, but not yet to the OS,
    // those are "posted" or "delayed" in case a conflicting request was posted
    std::mutex waiting_mtx_;
    queue_type waiting_requests_;

    std::mutex posted_mtx_;
    queue_type posted_requests_;
    queue_type delayed_requests_;
    queue_type local_queue;


    //! max number of OS requests
    int max_events_;
    //! number of requests in waitings_requests
    semaphore num_waiting_requests_, num_free_events_, num_posted_requests_;

    // two threads, one for posting, one for waiting
    std::thread post_thread_, wait_thread_;
    shared_state<thread_state> post_thread_state_, wait_thread_state_;

    // Why do we need two threads, one for posting, and one for waiting?  Is
    // one not enough?
    // 1. User call cannot io_submit directly, since this tends to take
    //    considerable time sometimes
    // 2. A single thread cannot wait for the user program to post requests
    //    and the OS to produce I/O completion events at the same time
    //    (IOCB_CMD_NOOP does not seem to help here either)

    static const priority_op priority_op_ = WRITE;

    static void * post_async(void* arg);   // thread start callback
    static void * wait_async(void* arg);   // thread start callback
    void post_requests();
    void handle_events(io_event* events, long num_events, bool canceled);
    void wait_requests();
    void suspend();

    // needed by linuxaio_request
    aio_context_t get_io_context() { return context_; }

#if FOXXLL_LINUXAIO_QUEUE_STATS
    std::atomic_int64_t stat_requests_added_ {0};
    std::atomic_int64_t stat_requests_delayed_ {0};
    std::atomic_int64_t stat_syscall_submit_ {0};
    std::atomic_int64_t stat_syscall_submit_repeat_ {0};
    std::atomic_int64_t stat_syscall_failed_post_ {0};
    std::atomic_int64_t stat_syscall_getevents_ {0};
    std::atomic_int64_t stat_syscall_cancel_ {0};
#endif

public:
    //! Construct queue. Requests max number of requests simultaneously
    //! submitted to disk, 0 means as many as possible
    explicit linuxaio_queue(int desired_queue_length = 0);

    void add_request(request_ptr& req) final;
    bool cancel_request(request_ptr& req) final;
    void complete_request(request_ptr& req);
    ~linuxaio_queue();
};

//! \}

} // namespace foxxll

#endif // #if STXXL_HAVE_LINUXAIO_FILE

#endif // !STXXL_IO_LINUXAIO_QUEUE_HEADER
// vim: et:ts=4:sw=4
