/***************************************************************************
 *  foxxll/io/request_queue_impl_qwqr.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <algorithm>
#include <functional>

#include <tlx/logger.hpp>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/io/request_queue_impl_qwqr.hpp>
#include <foxxll/io/serving_request.hpp>

#if FOXXLL_MSVC >= 1700 && FOXXLL_MSVC <= 1800
 #include <windows.h>
#endif

#ifndef FOXXLL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
#define FOXXLL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION 1
#endif

namespace foxxll {

struct file_offset_match
    : public std::binary_function<request_ptr, request_ptr, bool>
{
    bool operator () (
        const request_ptr& a,
        const request_ptr& b) const
    {
        // matching file and offset are enough to cause problems
        return (a->offset() == b->offset()) &&
               (a->get_file() == b->get_file());
    }
};

request_queue_impl_qwqr::request_queue_impl_qwqr(int n)
    : thread_state_(NOT_RUNNING), sem_(0)
{
    tlx::unused(n);
    start_thread(worker, static_cast<void*>(this), thread_, thread_state_);
}

void request_queue_impl_qwqr::set_priority_op(const priority_op& op)
{
    //_priority_op = op;
    tlx::unused(op);
}

void request_queue_impl_qwqr::add_request(request_ptr& req)
{
    if (req.empty())
        FOXXLL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (thread_state_() != RUNNING)
        FOXXLL_THROW_INVALID_ARGUMENT("Request submitted to not running queue.");
    if (!dynamic_cast<serving_request*>(req.get()))
        LOG1 << "Incompatible request submitted to running queue.";

    if (req.get()->op() == request::READ)
    {
#if FOXXLL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
        {
            std::unique_lock<std::mutex> lock(write_mutex_);
            if (std::find_if(write_queue_.begin(), write_queue_.end(),
                             bind2nd(file_offset_match(), req))
                != write_queue_.end())
            {
                LOG1 << "READ request submitted for a BID with a pending WRITE request";
            }
        }
#endif
        std::unique_lock<std::mutex> lock(read_mutex_);
        read_queue_.push_back(req);
    }
    else
    {
#if FOXXLL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
        {
            std::unique_lock<std::mutex> lock(read_mutex_);
            if (std::find_if(read_queue_.begin(), read_queue_.end(),
                             bind2nd(file_offset_match(), req))
                != read_queue_.end())
            {
                LOG1 << "WRITE request submitted for a BID with a pending READ request";
            }
        }
#endif
        std::unique_lock<std::mutex> lock(write_mutex_);
        write_queue_.push_back(req);
    }

    sem_.signal();
}

bool request_queue_impl_qwqr::cancel_request(request_ptr& req)
{
    if (req.empty())
        FOXXLL_THROW_INVALID_ARGUMENT("Empty request canceled disk_queue.");
    if (thread_state_() != RUNNING)
        FOXXLL_THROW_INVALID_ARGUMENT("Request canceled to not running queue.");
    if (!dynamic_cast<serving_request*>(req.get()))
        LOG1 << "Incompatible request submitted to running queue.";

    bool was_still_in_queue = false;
    if (req.get()->op() == request::READ)
    {
        std::unique_lock<std::mutex> lock(read_mutex_);
        queue_type::iterator pos
            = std::find(read_queue_.begin(), read_queue_.end(), req);
        if (pos != read_queue_.end())
        {
            read_queue_.erase(pos);
            was_still_in_queue = true;
            lock.unlock();
            sem_.wait();
        }
    }
    else
    {
        std::unique_lock<std::mutex> lock(write_mutex_);
        queue_type::iterator pos
            = std::find(write_queue_.begin(), write_queue_.end(), req);
        if (pos != write_queue_.end())
        {
            write_queue_.erase(pos);
            was_still_in_queue = true;
            lock.unlock();
            sem_.wait();
        }
    }

    return was_still_in_queue;
}

request_queue_impl_qwqr::~request_queue_impl_qwqr()
{
    stop_thread(thread_, thread_state_, sem_);
}

void* request_queue_impl_qwqr::worker(void* arg)
{
    self* pthis = static_cast<self*>(arg);

    bool write_phase = true;
    for ( ; ; )
    {
        pthis->sem_.wait();

        if (write_phase)
        {
            std::unique_lock<std::mutex> write_lock(pthis->write_mutex_);
            if (!pthis->write_queue_.empty())
            {
                request_ptr req = pthis->write_queue_.front();
                pthis->write_queue_.pop_front();

                write_lock.unlock();

                //assert(req->get_reference_count()) > 1);
                dynamic_cast<serving_request*>(req.get())->serve();
            }
            else
            {
                write_lock.unlock();

                pthis->sem_.signal();

                if (pthis->priority_op_ == WRITE)
                    write_phase = false;
            }

            if (pthis->priority_op_ == NONE || pthis->priority_op_ == READ)
                write_phase = false;
        }
        else
        {
            std::unique_lock<std::mutex> read_lock(pthis->read_mutex_);

            if (!pthis->read_queue_.empty())
            {
                request_ptr req = pthis->read_queue_.front();
                pthis->read_queue_.pop_front();

                read_lock.unlock();

                LOG << "queue: before serve request has "
                    << req->reference_count() << " references ";
                //assert(req->get_reference_count() > 1);
                dynamic_cast<serving_request*>(req.get())->serve();
                LOG << "queue: after serve request has "
                    << req->reference_count() << " references ";
            }
            else
            {
                read_lock.unlock();

                pthis->sem_.signal();

                if (pthis->priority_op_ == READ)
                    write_phase = true;
            }

            if (pthis->priority_op_ == NONE || pthis->priority_op_ == WRITE)
                write_phase = true;
        }

        // terminate if it has been requested and queues are empty
        if (pthis->thread_state_() == TERMINATING) {
            if (pthis->sem_.wait() == 0)
                break;
            else
                pthis->sem_.signal();
        }
    }

    pthis->thread_state_.set_to(TERMINATED);

#if FOXXLL_MSVC >= 1700 && FOXXLL_MSVC <= 1800
    // Workaround for deadlock bug in Visual C++ Runtime 2012 and 2013, see
    // request_queue_impl_worker.cpp. -tb
    ExitThread(nullptr);
#else
    return nullptr;
#endif
}

} // namespace foxxll
// vim: et:ts=4:sw=4
