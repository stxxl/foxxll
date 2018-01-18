/***************************************************************************
 *  foxxll/io/disk_queues.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_IO_DISK_QUEUES_HEADER
#define FOXXLL_IO_DISK_QUEUES_HEADER

#include <map>
#include <mutex>

#include <foxxll/io/iostats.hpp>
#include <foxxll/io/linuxaio_queue.hpp>
#include <foxxll/io/linuxaio_request.hpp>
#include <foxxll/io/request.hpp>
#include <foxxll/io/request_queue.hpp>
#include <foxxll/io/request_queue_impl_qwqr.hpp>
#include <foxxll/io/serving_request.hpp>
#include <foxxll/singleton.hpp>

namespace foxxll {

//! \addtogroup reqlayer
//! \{

//! Encapsulates disk queues.
//! \remark is a singleton
class disk_queues : public singleton<disk_queues>
{
    friend class singleton<disk_queues>;

    using disk_id_type = int64_t;
    using request_queue_map = std::map<disk_id_type, request_queue*>;

protected:
    std::mutex mutex_;

    request_queue_map queues_;

    disk_queues();

public:
    void make_queue(file* file);

    void add_request(request_ptr& req, disk_id_type disk);

    //! Cancel a request.
    //! The specified request is canceled unless already being processed.
    //! However, cancelation cannot be guaranteed.
    //! Cancelled requests must still be waited for in order to ensure correct
    //! operation.
    //! \param req request to cancel
    //! \param disk disk number for disk that \c req was scheduled on
    //! \return \c true iff the request was canceled successfully
    bool cancel_request(request_ptr& req, disk_id_type disk);

    request_queue * get_queue(disk_id_type disk);

    ~disk_queues();

    //! Changes requests priorities.
    //! \param op one of:
    //! - READ, read requests are served before write requests within a disk queue
    //! - WRITE, write requests are served before read requests within a disk queue
    //! - NONE, read and write requests are served by turns, alternately
    void set_priority_op(const request_queue::priority_op& op);
};

//! \}

} // namespace foxxll

#endif // !FOXXLL_IO_DISK_QUEUES_HEADER
// vim: et:ts=4:sw=4
