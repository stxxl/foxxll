/***************************************************************************
 *  foxxll/io/linuxaio_request.cpp
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

#include <foxxll/io/linuxaio_request.hpp>

#if STXXL_HAVE_LINUXAIO_FILE

#include <sys/syscall.h>
#include <unistd.h>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/io/disk_queues.hpp>
#include <foxxll/verbose.hpp>

namespace foxxll {

void linuxaio_request::completed(bool posted, bool canceled)
{
    STXXL_VERBOSE_LINUXAIO("linuxaio_request[" << this << "] completed(" <<
                           posted << "," << canceled << ")");

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

    cb_.aio_data = reinterpret_cast<__u64>(this);
    cb_.aio_fildes = af->file_des_;
    cb_.aio_lio_opcode = (op_ == READ) ? IOCB_CMD_PREAD : IOCB_CMD_PWRITE;
    cb_.aio_reqprio = 0;
    cb_.aio_buf = static_cast<__u64>(reinterpret_cast<unsigned long>(buffer_));
    cb_.aio_nbytes = bytes_;
    cb_.aio_offset = offset_;
}

void linuxaio_request::prepare_post()
{
    fill_control_block();
    time_posted_ = timestamp();
}

//! Cancel the request
//!
//! Routine is called by user, as part of the request interface.
bool linuxaio_request::cancel()
{
    STXXL_VERBOSE_LINUXAIO("linuxaio_request[" << this << "] cancel()");

    if (!file_) return false;

    request_ptr req(this);
    linuxaio_queue* queue = dynamic_cast<linuxaio_queue*>(
        disk_queues::get_instance()->get_queue(file_->get_queue_id()));
    return queue->cancel_request(req);
}

} // namespace foxxll

#endif // #if STXXL_HAVE_LINUXAIO_FILE
// vim: et:ts=4:sw=4
