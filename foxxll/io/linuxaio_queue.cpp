/***************************************************************************
 *  foxxll/io/linuxaio_queue.cpp
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

#include <foxxll/io/linuxaio_queue.hpp>

#if STXXL_HAVE_LINUXAIO_FILE

#include <foxxll/common/error_handling.hpp>
#include <foxxll/io/linuxaio_queue.hpp>
#include <foxxll/io/linuxaio_request.hpp>
#include <foxxll/mng/block_manager.hpp>
#include <foxxll/verbose.hpp>

#include <algorithm>

#include <sys/syscall.h>
#include <unistd.h>
#include <cstring>

#ifndef STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
#define STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION 1
#endif


#if FOXXLL_LINUXAIO_QUEUE_STATS
#define FOXXLL_LINUXAIO_IF_STAT(X) X
#else
#define FOXXLL_LINUXAIO_IF_STAT(X) {};
#endif

namespace foxxll {

linuxaio_queue::linuxaio_queue(int desired_queue_length)
    : num_waiting_requests_(0), num_free_events_(0), num_posted_requests_(0),
      post_thread_state_(NOT_RUNNING), wait_thread_state_(NOT_RUNNING)
{
    if (desired_queue_length == 0) {
        // default value, 64 entries per queue (i.e. usually per disk) should
        // be enough
        max_events_ = 64;
    }
    else
        max_events_ = desired_queue_length;

    // negotiate maximum number of simultaneous events with the OS
    context_ = 0;
    long result;
    while ((result = syscall(SYS_io_setup, max_events_, &context_)) == -1 &&
           errno == EAGAIN && max_events_ > 1)
    {
        max_events_ <<= 1;               // try with half as many events
    }
    if (result != 0) {
        STXXL_THROW_ERRNO(io_error, "linuxaio_queue::linuxaio_queue"
                          " io_setup() nr_events=" << max_events_);
    }

    num_free_events_.signal(max_events_);

    STXXL_MSG("Set up an linuxaio queue with " << max_events_ << " entries.");

    start_thread(post_async, static_cast<void*>(this), post_thread_, post_thread_state_);
    start_thread(wait_async, static_cast<void*>(this), wait_thread_, wait_thread_state_);
}

linuxaio_queue::~linuxaio_queue()
{
    stop_thread(post_thread_, post_thread_state_, num_waiting_requests_);
    stop_thread(wait_thread_, wait_thread_state_, num_posted_requests_);
    syscall(SYS_io_destroy, context_);

#if FOXXLL_LINUXAIO_QUEUE_STATS
    STXXL_MSG(
        "LinuxAIO stats:\n"
        " requests:         " << stat_requests_added_ << "\n"
        " submit:           " << stat_syscall_submit_ << "\n"
        " reqs/submit:      " << (static_cast<double>(stat_requests_added_) / stat_syscall_submit_) << "\n"
        " delayed:          " << stat_requests_delayed_ << "\n"
        " submit(repeated): " << stat_syscall_submit_repeat_ << "\n"
        " submit(empty)   : " << stat_syscall_failed_post_ << "\n"
        " getevents:        " << stat_syscall_getevents_ << "\n"
        " cancel:           " << stat_syscall_cancel_ << "\n"
    );
#endif
}

void linuxaio_queue::add_request(request_ptr& req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (post_thread_state_() != RUNNING)
        STXXL_ERRMSG("Request submitted to stopped queue.");
    if (!dynamic_cast<linuxaio_request*>(req.get()))
        STXXL_ERRMSG("Non-LinuxAIO request submitted to LinuxAIO queue.");

    std::unique_lock<std::mutex> lock(waiting_mtx_);

    waiting_requests_.push_back(req);
    FOXXLL_LINUXAIO_IF_STAT(stat_requests_added_++);
    num_waiting_requests_.signal();
}

bool linuxaio_queue::cancel_request(request_ptr& req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request canceled disk_queue.");
    if (post_thread_state_() != RUNNING)
        STXXL_ERRMSG("Request canceled in stopped queue.");

    auto* aio_req = dynamic_cast<linuxaio_request *>(req.get());
    if (!aio_req)
        STXXL_ERRMSG("Non-LinuxAIO request submitted to LinuxAIO queue.");

    {
        std::unique_lock<std::mutex> lock(waiting_mtx_);

        // check waiting requests
        {
            auto pos = std::find(waiting_requests_.begin(), waiting_requests_.end(), req);
            if (pos != waiting_requests_.end()) {
                waiting_requests_.erase(pos);
                aio_req->completed(false, true);
                num_waiting_requests_.wait(); // will never block
                return true;
            }
        }

        // check waiting requests
        {
            auto pos = std::find(delayed_requests_.begin(), delayed_requests_.end(), req);
            if (pos != delayed_requests_.end()) {
                delayed_requests_.erase(pos);
                aio_req->completed(false, true);
                num_waiting_requests_.wait(); // will never block
                return true;
            }
        }
    }

    {
        auto pos = std::find(posted_requests_.begin(), posted_requests_.end(), req);
        if (pos != posted_requests_.end()) {
            // Try to cancel posted request
            {
                STXXL_VERBOSE_LINUXAIO("linuxaio_request[" << this << "] cancel_aio()");

                if (!req->get_file()) return false;

                io_event event;
                long result = syscall(SYS_io_cancel, context_, &aio_req->control_block(), &event);
                FOXXLL_LINUXAIO_IF_STAT(stat_syscall_cancel_++);
                if (result == 0) {
                    //successfully canceled
                    handle_events(&event, 1, true);
                } else {
                    return false;
                }
            }

            // Clean up queue
            {
                posted_requests_.erase(pos);

                // request is canceled, already posted
                aio_req->completed(true, true);

                num_free_events_.signal();
                num_posted_requests_.wait(); // will never block
                return true;
            }
        }
    }

    return false;
}

// internal routines, run by the posting thread
void linuxaio_queue::post_requests()
{
    std::unique_ptr<io_event[]> events(new io_event[max_events_]);
    std::unique_ptr<iocb*[]>    cbps(new iocb*[max_events_]);

    for ( ; ; ) // as long as thread is running
    {
        // might block until next request or message comes in
        size_t num_currently_waiting_requests = num_waiting_requests_.wait();

        // terminate if termination has been requested
        if (post_thread_state_() == TERMINATING && num_currently_waiting_requests == 0)
            break;

        num_currently_waiting_requests++;

        const size_t free_events = num_free_events_.wait(); // might block because too many requests are posted

        bool progress_in_this_round = false;

        std::unique_lock<std::mutex> lock_wait(waiting_mtx_);
        if (!waiting_requests_.empty() || !delayed_requests_.empty())
        {
            {
                // taking this lock for a little bit longer should not be an issue as only manually
                // cancelling a request requires it otherwise
                std::unique_lock<std::mutex> lock_post(posted_mtx_);

                // first of all: remove all completed requests from posted -- the queue does not need them anymore
                posted_requests_.remove_if([] (const auto& x) {return x.get()->poll();});

                // we have a conflict if two requests target the same range and at least one is a write
                auto is_conflicting = [](const queue_type &queue, const request &req) {
                    auto in_conflict = [&req](const request_ptr& o) {
                        return req.overlaps_with(*(o.get()))
                               && !(req.get_op() == request::READ && o.get()->get_op() == request::READ );};

                    auto it = std::find_if(queue.cbegin(), queue.cend(), in_conflict);
                    return it != queue.cend();
                };


                // at first we cherry-pick all delayed requests that are not conflicting anymore
                for (auto it = delayed_requests_.begin(); it != delayed_requests_.cend() && local_queue.size() < free_events;) {
                    const request &req = *(it->get());

                    if (is_conflicting(posted_requests_, req)) {
                        std::advance(it, 1);
                        continue;
                    }

                    auto next = std::next(it);
                    local_queue.splice(local_queue.end(), delayed_requests_, it, next);
                    it = next;
                }

                // now we fill the remaining slots with waiting requests (or move them to delayed if they are conflicting)
                bool not_first = false;
                while (local_queue.size() < free_events && !waiting_requests_.empty() && num_currently_waiting_requests--) {
                    const request &req = *(waiting_requests_.front().get());

                    if (is_conflicting(posted_requests_, req) ||
                        is_conflicting(local_queue, req)) {
                        STXXL_VERBOSE_LINUXAIO("linuxaio_queue::post_requests [" << &req << "] delayed");
                        FOXXLL_LINUXAIO_IF_STAT(stat_requests_delayed_++);
                        delayed_requests_.splice(delayed_requests_.end(), waiting_requests_, waiting_requests_.begin(), std::next(waiting_requests_.begin()));
                        continue;
                    }

                    // keep semaphore consistent
                    if (not_first) {
                        num_waiting_requests_.wait(); // will never block
                    }
                    not_first = true;

                    local_queue.splice(local_queue.end(), waiting_requests_, waiting_requests_.begin(), std::next(waiting_requests_.begin()));
                }
            }
            lock_wait.unlock();

            if (!local_queue.empty()) {
                {
                    size_t i = 0;
                    for (request_ptr &ptr : local_queue) {
                        auto &req = *dynamic_cast<linuxaio_request *>(ptr.get());
                        req.prepare_post();
                        cbps[i] = &req.control_block();

                        if (i++) num_free_events_.wait(); // will never block
                    }
                }

                size_t already_posted = 0;
                while (!local_queue.empty()) {
                    long result = syscall(SYS_io_submit, context_,
                                          local_queue.size(), // number of requests to post
                                          (cbps.get() + already_posted) // pointer to their control blocks
                    );
                    FOXXLL_LINUXAIO_IF_STAT(stat_syscall_submit_++);
                    STXXL_VERBOSE_LINUXAIO("Posted " << local_queue.size() << " requests; result=" << result);

                    if (result > 0) {
                        // positive value indicates that (some) requests were posted
                        assert(static_cast<size_t>(result) <= local_queue.size());

                        already_posted += result;
                        {
                            std::unique_lock<std::mutex> lock_post(posted_mtx_);
                            posted_requests_.splice(posted_requests_.cend(), local_queue, local_queue.begin(),
                                                    std::next(local_queue.begin(), result));
                            num_posted_requests_.signal(result);
                        }

                        continue;
                    }

                    if (result == -1 && errno != EAGAIN) {
                        STXXL_MSG("linuxaio_queue::post io_submit() nr=" << errno);
                        abort();
                    }

                    FOXXLL_LINUXAIO_IF_STAT(stat_syscall_submit_repeat_++);

                    // post failed, so first handle events to make queues (more)
                    // empty, then try again.

                    // wait for at least one event to complete, no time limit
                    long num_events = syscall(SYS_io_getevents, context_, 1, max_events_, events.get(), nullptr);
                    FOXXLL_LINUXAIO_IF_STAT(stat_syscall_getevents_++);
                    if (num_events < 0) {
                        STXXL_THROW_ERRNO(io_error, "linuxaio_queue::post_requests"
                            " io_getevents() nr_events=" << num_events);
                    }

                    handle_events(events.get(), num_events, false);
                }

                progress_in_this_round = true;
            }
        } else {
            lock_wait.unlock();
        }

        if (!progress_in_this_round)
        {
            // semaphores where decremented prematurely; compensate
            FOXXLL_LINUXAIO_IF_STAT(stat_syscall_failed_post_++);
            num_waiting_requests_.signal();
            num_free_events_.signal();
        }
    }
}

void linuxaio_queue::handle_events(io_event* events, long num_events, bool canceled)
{
    // We refrain from removing entries from posted_requests at this point since due to a data race,
    // this function may be called BEFORE the request gets ever enqueued. We hence only mark a request
    // as completed/cancelled and clean up the queue in the posting-thread.
    for (int e = 0; e < num_events; ++e)
    {
        // size_t is as long as a pointer, and like this, we avoid an icpc warning
        reinterpret_cast<linuxaio_request*>(static_cast<size_t>(events[e].data))->completed(canceled);

        num_free_events_.signal();
        num_posted_requests_.wait(); // will never block
    }
}

// internal routines, run by the waiting thread
void linuxaio_queue::wait_requests()
{
    std::unique_ptr<io_event[]> events(new io_event[max_events_]);

    for ( ; ; ) // as long as thread is running
    {
        // might block until next request is posted or message comes in
        int num_currently_posted_requests = num_posted_requests_.wait();

        // terminate if termination has been requested
        if (wait_thread_state_() == TERMINATING && num_currently_posted_requests == 0)
            break;

        // wait for at least one of them to finish
        long num_events = -1;
        while (num_events < 0) {
            num_events = syscall(SYS_io_getevents, context_, 1, max_events_, events.get(), nullptr);
            FOXXLL_LINUXAIO_IF_STAT(stat_syscall_getevents_++);

            if (num_events < 0) {
                if (errno == EINTR) {
                    // io_getevents may return prematurely in case a signal is received. This is
                    // specified behaviour and only requires that we try it again.
                    continue;
                }

                STXXL_THROW_ERRNO(io_error, "linuxaio_queue::wait_requests"
                                  " io_getevents() nr_events=" << max_events_);
            }
        }

        num_posted_requests_.signal(); // compensate for the one eaten prematurely above

        handle_events(events.get(), num_events, false);
    }
}

void* linuxaio_queue::post_async(void* arg)
{
    (static_cast<linuxaio_queue*>(arg))->post_requests();

    self_type* pthis = static_cast<self_type*>(arg);
    pthis->post_thread_state_.set_to(TERMINATED);

#if STXXL_MSVC >= 1700
    // Workaround for deadlock bug in Visual C++ Runtime 2012 and 2013, see
    // request_queue_impl_worker.cpp. -tb
    ExitThread(nullptr);
#else
    return nullptr;
#endif
}

void* linuxaio_queue::wait_async(void* arg)
{
    (static_cast<linuxaio_queue*>(arg))->wait_requests();

    self_type* pthis = static_cast<self_type*>(arg);
    pthis->wait_thread_state_.set_to(TERMINATED);

#if STXXL_MSVC >= 1700
    // Workaround for deadlock bug in Visual C++ Runtime 2012 and 2013, see
    // request_queue_impl_worker.cpp. -tb
    ExitThread(nullptr);
#else
    return nullptr;
#endif
}

} // namespace foxxll

#undef FOXXLL_LINUXAIO_IF_STAT

#endif // #if STXXL_HAVE_LINUXAIO_FILE
// vim: et:ts=4:sw=4
