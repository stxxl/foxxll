/***************************************************************************
 *  foxxll/io/linuxaio_queue.cpp
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

#include <foxxll/io/linuxaio_queue.hpp>

#if STXXL_HAVE_LINUXAIO_FILE

#include <sys/syscall.h>
#include <unistd.h>

#include <cstring>

#include <algorithm>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/io/linuxaio_request.hpp>
#include <foxxll/mng/block_manager.hpp>
#include <foxxll/verbose.hpp>

#ifndef STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
#define STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION 1
#endif

#if FOXXLL_LINUXAIO_QUEUE_STATS
#define FOXXLL_LINUXAIO_IF_STAT(X) X
#else
#define FOXXLL_LINUXAIO_IF_STAT(X) { };
#endif

namespace foxxll {

linuxaio_queue::linuxaio_queue(size_t desired_queue_length)
    : os_queue_length_(setup_context(desired_queue_length))
{
    STXXL_MSG("Set up an linuxaio queue with " << os_queue_length_ << " entries.");

    // Workaround for deadlock bug in Visual C++ Runtime 2012 and 2013, see
    // request_queue_impl_worker.cpp. -tb
    auto graceful_thread_death =
        []() {
#if STXXL_MSVC >= 1700
            ExitThread(nullptr);
#endif
            return nullptr;
        };

    start_thread([&](void*) -> void* {
                     post_requests();
                     post_thread_state_.set_to(TERMINATED);
                     return graceful_thread_death();
                 }, nullptr, post_thread_, post_thread_state_);

    start_thread([&](void*) -> void* {
                     wait_requests();
                     wait_thread_state_.set_to(TERMINATED);
                     return graceful_thread_death();
                 }, nullptr, wait_thread_, wait_thread_state_);
}

linuxaio_queue::~linuxaio_queue()
{
    stop_thread_with_callback(post_thread_, post_thread_state_, [this]() {
                                  std::unique_lock<std::mutex> lock_wait(waiting_mtx_);
                                  post_thread_cv_.notify_one();
                              });

    stop_thread_with_callback(wait_thread_, wait_thread_state_, [this]() {
                                  std::unique_lock<std::mutex> lock_post(posted_mtx_);
                                  wait_thread_cv_.notify_one();
                              });

    syscall(SYS_io_destroy, context_);

#if FOXXLL_LINUXAIO_QUEUE_STATS
    STXXL_MSG(
        "linuxaio stats:\n"
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

size_t linuxaio_queue::setup_context(size_t requested)
{
    size_t queue_length = requested ? requested : 64;
    if (queue_length > std::numeric_limits<unsigned>::max())
        queue_length = std::numeric_limits<unsigned>::max();

    context_ = 0;

    // negotiate maximum number of simultaneous events with the OS
    while (true) {
        long result = syscall(SYS_io_setup, queue_length, &context_);

        if (result == 0) {
            // success!
            return queue_length;
        }

        if (result == -1 && errno == EAGAIN && queue_length > 1) {
            // failed, but we may try again with a shorter queue
            continue;
        }

        STXXL_THROW_ERRNO(io_error, "linuxaio_queue::linuxaio_queue"
                          " io_setup() nr_events=" << queue_length);
    }
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

    post_thread_cv_.notify_one();
}

bool linuxaio_queue::cancel_request(request_ptr& req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request canceled disk_queue.");
    if (post_thread_state_() != RUNNING)
        STXXL_ERRMSG("Request canceled in stopped queue.");

    auto* aio_req = dynamic_cast<linuxaio_request*>(req.get());
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
                return true;
            }
        }

        // check waiting requests
        {
            auto pos = std::find(delayed_requests_.begin(), delayed_requests_.end(), req);
            if (pos != delayed_requests_.end()) {
                delayed_requests_.erase(pos);
                aio_req->completed(false, true);
                return true;
            }
        }
    }

    {
        std::unique_lock<std::mutex> lock(posted_mtx_);

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
                }
                else {
                    return false;
                }
            }
        }
    }

    return false;
}

// internal routines, run by the posting thread
void linuxaio_queue::post_requests()
{
    queue_type local_queue;

    std::unique_ptr<io_event[]> events(new io_event[os_queue_length_]);
    std::unique_ptr<iocb*[]> cbps(new iocb*[os_queue_length_]);

    for ( ; ; ) // as long as thread is running
    {
        std::unique_lock<std::mutex> lock_wait(waiting_mtx_);
        post_thread_cv_.wait(lock_wait, [this]() {
                                 return ((!waiting_requests_.empty() || !delayed_requests_.empty()) && no_requests_posted_ < static_cast<int64_t>(os_queue_length_))
                                 || (post_thread_state_() == TERMINATING);
                             });

        // terminate if termination has been requested
        if (post_thread_state_() == TERMINATING
            && waiting_requests_.empty()
            && delayed_requests_.empty())
            break;

        {
            // taking this lock for a little bit longer should not be an issue as only manually
            // cancelling a request requires it otherwise
            std::unique_lock<std::mutex> lock_post(posted_mtx_);

            // first of all: remove all completed requests from posted -- the queue does not need them anymore
            posted_requests_.remove_if([](const auto& x) { return x.get()->poll(); });

            // we have a conflict if two requests target the same range and at least one is a write
            auto is_conflicting =
                [](const queue_type& queue, const request& req) {
                    auto in_conflict = [&req](const request_ptr& o) {
                                           return req.overlaps_with(*(o.get()))
                                                  && !(req.op() == request::READ && o.get()->op() == request::READ);
                                       };

                    auto it = std::find_if(queue.cbegin(), queue.cend(), in_conflict);
                    return it != queue.cend();
                };

            auto local_queue_at_max_size = [&]() {
                                               return static_cast<int64_t>(local_queue.size()) + no_requests_posted_ >= static_cast<int64_t>(os_queue_length_);
                                           };

            // at first we cherry-pick all delayed requests that are not conflicting anymore
            for (auto it = delayed_requests_.begin(); it != delayed_requests_.cend() && !local_queue_at_max_size(); ) {
                const request& req = *(it->get());

                if (is_conflicting(posted_requests_, req)) {
                    std::advance(it, 1);
                    continue;
                }

                auto next = std::next(it);
                local_queue.splice(local_queue.end(), delayed_requests_, it, next);
                it = next;
            }

            // now we fill the remaining slots with waiting requests (or move them to delayed if they are conflicting)
            while (!local_queue_at_max_size() && !waiting_requests_.empty()) {
                const request& req = *(waiting_requests_.front().get());

                if (is_conflicting(posted_requests_, req) ||
                    is_conflicting(local_queue, req)) {
                    STXXL_VERBOSE_LINUXAIO("linuxaio_queue::post_requests [" << &req << "] delayed");
                    FOXXLL_LINUXAIO_IF_STAT(stat_requests_delayed_++);
                    delayed_requests_.splice(delayed_requests_.end(), waiting_requests_,
                                             waiting_requests_.begin(), std::next(waiting_requests_.begin()));
                    continue;
                }

                local_queue.splice(local_queue.end(), waiting_requests_,
                                   waiting_requests_.begin(), std::next(waiting_requests_.begin()));
            }
        }
        lock_wait.unlock();

        if (local_queue.empty())
            continue;

        // prepare the syscall by populating the necesary control blocks (linuxaio_request::prepare_post)
        // and the cbps data structure pointing to those
        {
            size_t i = 0;
            for (request_ptr& ptr : local_queue) {
                auto& req = *dynamic_cast<linuxaio_request*>(ptr.get());
                req.prepare_post();
                cbps[i++] = &req.control_block();
            }
        }

        // there is a possibility that the kernel does not submit all (or any) requests;
        // in this case we simply have to resubmit the not yet processes blocks
        size_t already_posted = 0;
        while (!local_queue.empty()) {
            long result = syscall(SYS_io_submit, context_,
                                  local_queue.size(),           // number of requests to post
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
                    no_requests_posted_ += result;

                    posted_requests_.splice(posted_requests_.cend(), local_queue, local_queue.begin(),
                                            std::next(local_queue.begin(), result));
                    wait_thread_cv_.notify_one();
                }

                continue;
            }

            if (result == -1 && errno != EAGAIN) {
                STXXL_MSG("linuxaio_queue::post io_submit() nr=" << errno << ", msg=" << std::strerror(errno));
                abort();
            }

            FOXXLL_LINUXAIO_IF_STAT(stat_syscall_submit_repeat_++);

            // post failed, so first handle events to make queues (more)
            // empty, then try again.

            // wait for at least one event to complete, no time limit
            long num_events = syscall(SYS_io_getevents, context_, 1, os_queue_length_, events.get(), nullptr);
            FOXXLL_LINUXAIO_IF_STAT(stat_syscall_getevents_++);
            if (num_events < 0) {
                STXXL_THROW_ERRNO(io_error, "linuxaio_queue::post_requests"
                                  " io_getevents() nr_events=" << num_events);
            }

            handle_events(events.get(), num_events, false);
        }
    }
}

void linuxaio_queue::handle_events(io_event* events, long num_events, bool canceled)
{
    // We refrain from removing entries from posted_requests at this point since due to a data race,
    // this function may be called BEFORE the request gets ever enqueued. We hence only mark a request
    // as completed/cancelled and clean up the queue in the posting-thread.
    for (int e = 0; e < num_events; ++e) {
        reinterpret_cast<linuxaio_request*>(static_cast<size_t>(events[e].data))->completed(canceled);
    }

    no_requests_posted_ -= num_events;

    std::unique_lock<std::mutex> lock_wait(waiting_mtx_);
    post_thread_cv_.notify_one();
}

// internal routines, run by the waiting thread
void linuxaio_queue::wait_requests()
{
    std::unique_ptr<io_event[]> events(new io_event[os_queue_length_]);

    for ( ; ; ) // as long as thread is running
    {
        {
            std::unique_lock<std::mutex> lock_post(posted_mtx_);
            wait_thread_cv_.wait(lock_post, [this]() {
                                     return (no_requests_posted_ > 0) || (wait_thread_state_() == TERMINATING);
                                 });
        }

        // terminate if termination has been requested
        // TODO: Also check that no requests are waiting
        if (wait_thread_state_() == TERMINATING && no_requests_posted_ == 0)
            break;

        long num_events = syscall(SYS_io_getevents, context_, 1, os_queue_length_, events.get(), nullptr);
        FOXXLL_LINUXAIO_IF_STAT(stat_syscall_getevents_++);

        if (num_events < 0) {
            // The only error code which actually does not indicate an error is EINTR;
            // this interuption is commonly caused by signals sent to us, e.g., by gdb
            // We hence just poll again; otherwise report the error to the user
            if (errno != EINTR) {
                STXXL_THROW_ERRNO(io_error, "linuxaio_queue::wait_requests"
                                  " io_getevents() nr_events=" << os_queue_length_);
            }

            continue;
        }

        handle_events(events.get(), num_events, false);
    }
}

} // namespace foxxll

#undef FOXXLL_LINUXAIO_IF_STAT

#endif // #if STXXL_HAVE_LINUXAIO_FILE
// vim: et:ts=4:sw=4
