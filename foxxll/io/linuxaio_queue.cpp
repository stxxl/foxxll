/***************************************************************************
 *  foxxll/io/linuxaio_queue.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
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

#if FOXXLL_HAVE_LINUXAIO_FILE

#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>

#include <tlx/define/likely.hpp>
#include <tlx/die/core.hpp>
#include <tlx/logger/core.hpp>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/io/linuxaio_request.hpp>
#include <foxxll/mng/block_manager.hpp>

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
        FOXXLL_THROW_ERRNO(
            io_error, "linuxaio_queue::linuxaio_queue"
            " io_setup() nr_events=" << max_events_
        );
    }

    num_free_events_.signal(max_events_);

    TLX_LOG1 << "Set up an linuxaio queue with " << max_events_ << " entries.";

    start_thread(post_async, static_cast<void*>(this), post_thread_, post_thread_state_);
    start_thread(wait_async, static_cast<void*>(this), wait_thread_, wait_thread_state_);
}

linuxaio_queue::~linuxaio_queue()
{
    stop_thread(post_thread_, post_thread_state_, num_waiting_requests_);
    stop_thread(wait_thread_, wait_thread_state_, num_posted_requests_);
    syscall(SYS_io_destroy, context_);
}

void linuxaio_queue::add_request(request_ptr& req)
{
    if (req.empty())
        FOXXLL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (post_thread_state_() != RUNNING)
        tlx_die("Request submitted to stopped queue.");
    if (!dynamic_cast<linuxaio_request*>(req.get()))
        tlx_die("Non-LinuxAIO request submitted to LinuxAIO queue.");

    std::unique_lock<std::mutex> lock(waiting_mtx_);
    waiting_requests_.push_back(req);
    lock.unlock();

    num_waiting_requests_.signal();
}

bool linuxaio_queue::cancel_request(request_ptr& req)
{
    if (req.empty())
        FOXXLL_THROW_INVALID_ARGUMENT("Empty request canceled disk_queue.");
    if (post_thread_state_() != RUNNING)
        tlx_die("Request canceled in stopped queue.");

    linuxaio_request* areq = dynamic_cast<linuxaio_request*>(req.get());
    if (!areq)
        tlx_die("Non-LinuxAIO request submitted to LinuxAIO queue.");

    queue_type::iterator pos;
    {
        std::unique_lock<std::mutex> lock(waiting_mtx_);

        pos = std::find(
                waiting_requests_.begin(), waiting_requests_.end(), req
            );
        if (pos != waiting_requests_.end())
        {
            waiting_requests_.erase(pos);
            lock.unlock();

            // request is canceled, but was not yet posted.
            areq->completed(false, true);

            num_waiting_requests_.wait(); // will never block
            return true;
        }
    }

    std::unique_lock<std::mutex> lock(waiting_mtx_);

    // perform syscall to cancel I/O
    bool canceled_io_operation = areq->cancel_aio(this);

    if (canceled_io_operation)
    {
        lock.unlock();
        num_free_events_.signal();

        // request is canceled, already posted, but canceled in kernel
        areq->completed(true, true);

        num_posted_requests_.wait(); // will never block
        return true;
    }

    return false;
}

// internal routines, run by the posting thread
void linuxaio_queue::post_requests()
{
    tlx::simple_vector<io_event> events(max_events_);

    for ( ; ; ) // as long as thread is running
    {
        // block until next request or message comes in
        int num_currently_waiting_requests = num_waiting_requests_.wait();

        // terminate if termination has been requested
        if (post_thread_state_() == TERMINATING &&
            num_currently_waiting_requests == 0)
            break;

        std::unique_lock<std::mutex> lock(waiting_mtx_);
        if (TLX_UNLIKELY(waiting_requests_.empty())) {
            // unlock queue
            lock.unlock();

            // num_waiting_requests_-- was premature, compensate for that
            num_waiting_requests_.signal();
            continue;
        }

        // collect requests from waiting queue: first is there
        std::vector<request_ptr> reqs;

        request_ptr req = waiting_requests_.front();
        waiting_requests_.pop_front();
        reqs.emplace_back(std::move(req));

        // collect additional requests
        while (!waiting_requests_.empty()) {
            // acquire one free event, but keep one in slack
            if (!num_free_events_.try_acquire(/* delta */ 1, /* slack */ 1))
                break;
            if (!num_waiting_requests_.try_acquire()) {
                num_free_events_.signal();
                break;
            }

            request_ptr req = waiting_requests_.front();
            waiting_requests_.pop_front();
            reqs.emplace_back(std::move(req));
        }

        lock.unlock();

        // the last free_event must be acquired outside of the lock.
        num_free_events_.wait();

        // construct batch iocb
        tlx::simple_vector<iocb*> cbs(reqs.size());

        for (size_t i = 0; i < reqs.size(); ++i) {
            // polymorphic_downcast
            auto ar = dynamic_cast<linuxaio_request*>(reqs[i].get());
            cbs[i] = ar->fill_control_block();
        }
        reqs.clear();

        // io_submit loop
        size_t cb_done = 0;
        while (cb_done < cbs.size()) {
            long success = syscall(
                    SYS_io_submit, context_,
                    cbs.size() - cb_done,
                    cbs.data() + cb_done
                );

            if (success <= 0 && errno != EAGAIN) {
                FOXXLL_THROW_ERRNO(
                    io_error, "linuxaio_request::post io_submit()"
                );
            }
            if (success > 0) {
                // request is posted
                num_posted_requests_.signal(success);

                cb_done += success;
                if (cb_done == cbs.size())
                    break;
            }

            // post failed, so first handle events to make queues (more) empty,
            // then try again.

            // wait for at least one event to complete, no time limit
            long num_events = syscall(
                    SYS_io_getevents, context_, 0,
                    max_events_, events.data(), nullptr
                );
            if (num_events < 0) {
                FOXXLL_THROW_ERRNO(
                    io_error, "linuxaio_queue::post_requests"
                    " io_getevents() nr_events=" << num_events
                );
            }
            if (num_events > 0)
                handle_events(events.data(), num_events, false);
        }
    }
}

void linuxaio_queue::handle_events(io_event* events, long num_events, bool canceled)
{
    // first mark all events as free
    num_free_events_.signal(num_events);

    for (int e = 0; e < num_events; ++e)
    {
        request* r = reinterpret_cast<request*>(
                static_cast<uintptr_t>(events[e].data));
        r->completed(canceled);
        // release counting_ptr reference, this may delete the request object
        r->dec_reference();
    }

    num_posted_requests_.wait(num_events); // will never block
}

// internal routines, run by the waiting thread
void linuxaio_queue::wait_requests()
{
    tlx::simple_vector<io_event> events(max_events_);

    for ( ; ; ) // as long as thread is running
    {
        // might block until next request is posted or message comes in
        int num_currently_posted_requests = num_posted_requests_.wait();

        // terminate if termination has been requested
        if (wait_thread_state_() == TERMINATING &&
            num_currently_posted_requests == 0)
            break;

        // wait for at least one of them to finish
        long num_events;
        while (1) {
            num_events = syscall(
                    SYS_io_getevents, context_, 1,
                    max_events_, events.data(), nullptr
                );

            if (num_events < 0) {
                if (errno == EINTR) {
                    // io_getevents may return prematurely in case a signal is
                    // received
                    continue;
                }

                FOXXLL_THROW_ERRNO(
                    io_error, "linuxaio_queue::wait_requests"
                    " io_getevents() nr_events=" << max_events_
                );
            }
            break;
        }

        // compensate for the one eaten prematurely above
        num_posted_requests_.signal();

        handle_events(events.data(), num_events, false);
    }
}

void* linuxaio_queue::post_async(void* arg)
{
    (static_cast<linuxaio_queue*>(arg))->post_requests();

    self_type* pthis = static_cast<self_type*>(arg);
    pthis->post_thread_state_.set_to(TERMINATED);

#if FOXXLL_MSVC >= 1700 && FOXXLL_MSVC <= 1800
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

#if FOXXLL_MSVC >= 1700 && FOXXLL_MSVC <= 1800
    // Workaround for deadlock bug in Visual C++ Runtime 2012 and 2013, see
    // request_queue_impl_worker.cpp. -tb
    ExitThread(nullptr);
#else
    return nullptr;
#endif
}

} // namespace foxxll

#endif // #if FOXXLL_HAVE_LINUXAIO_FILE

/**************************************************************************/
