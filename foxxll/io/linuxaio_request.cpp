/***************************************************************************
 *  foxxll/io/linuxaio_request.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2011 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <foxxll/io/linuxaio_request.hpp>

#if FOXXLL_HAVE_LINUXAIO_FILE

#include <sys/syscall.h>
#include <unistd.h>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/io/disk_queues.hpp>

namespace foxxll {

void linuxaio_request::completed(bool posted, bool canceled)
{
    LOG << "linuxaio_request[" << this << "] completed(" <<
        posted << "," << canceled << ")";

    auto* stats = file_->get_file_stats();
    const double duration = timestamp() - time_posted_;

    if (!canceled)
    {
        if (op_ == READ) {
            stats->read_op_finished(bytes_, duration);
        }
        else {
            stats->write_op_finished(bytes_, duration);
        }
    }
    else if (posted)
    {
        if (op_ == READ)
            stats->read_canceled(bytes_);
        else
            stats->write_canceled(bytes_);
    }

    request_with_state::completed(canceled);
}

void linuxaio_request::fill_control_block()
{
    linuxaio_file* af = dynamic_cast<linuxaio_file*>(file_);

    memset(&cb_, 0, sizeof(cb_));
    // indirection, I/O system retains a virtual counting_ptr reference
    ReferenceCounter::inc_reference();
    cb_.aio_data = reinterpret_cast<__u64>(this);
    cb_.aio_fildes = af->file_des_;
    cb_.aio_lio_opcode = (op_ == READ) ? IOCB_CMD_PREAD : IOCB_CMD_PWRITE;
    cb_.aio_reqprio = 0;
    cb_.aio_buf = static_cast<__u64>(reinterpret_cast<unsigned long>(buffer_));
    cb_.aio_nbytes = bytes_;
    cb_.aio_offset = offset_;
}

//! Submits an I/O request to the OS
//! \returns false if submission fails
bool linuxaio_request::post()
{
    LOG << "linuxaio_request[" << this << "] post()";

    fill_control_block();
    iocb* cb_pointer = &cb_;
    // io_submit might considerable time, so we have to remember the current
    // time before the call.
    time_posted_ = timestamp();
    linuxaio_queue* queue = dynamic_cast<linuxaio_queue*>(
            disk_queues::get_instance()->get_queue(file_->get_queue_id()));

    long success = syscall(SYS_io_submit, queue->get_io_context(), 1, &cb_pointer);
    // At this point another thread may have already called complete(),
    // so consider most values as invalidated!

    if (success == -1 && errno != EAGAIN)
        FOXXLL_THROW_ERRNO(
            io_error, "linuxaio_request::post"
            " io_submit()"
        );

    return success == 1;
}

//! Cancel the request
//!
//! Routine is called by user, as part of the request interface.
bool linuxaio_request::cancel()
{
    LOG << "linuxaio_request[" << this << "] cancel()";

    if (!file_) return false;

    request_ptr req(this);
    linuxaio_queue* queue = dynamic_cast<linuxaio_queue*>(
            disk_queues::get_instance()->get_queue(file_->get_queue_id()));
    return queue->cancel_request(req);
}

//! Cancel already posted request
bool linuxaio_request::cancel_aio(linuxaio_queue* queue)
{
    LOG << "linuxaio_request[" << this << "] cancel_aio()";

    if (!file_) return false;

    io_event event;
    long result = syscall(SYS_io_cancel, queue->get_io_context(), &cb_, &event);
    if (result == 0)    //successfully canceled
        queue->handle_events(&event, 1, true);
    return result == 0;
}

} // namespace foxxll

#endif // #if FOXXLL_HAVE_LINUXAIO_FILE

/**************************************************************************/
