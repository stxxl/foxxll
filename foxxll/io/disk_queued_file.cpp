/***************************************************************************
 *  foxxll/io/disk_queued_file.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <foxxll/io/disk_queued_file.hpp>
#include <foxxll/io/disk_queues.hpp>
#include <foxxll/io/file.hpp>
#include <foxxll/io/request.hpp>
#include <foxxll/io/request_interface.hpp>
#include <foxxll/io/serving_request.hpp>
#include <foxxll/singleton.hpp>

namespace foxxll {

request_ptr disk_queued_file::aread(
    void* buffer, offset_type offset, size_type bytes,
    const completion_handler& on_complete)
{
    request_ptr req = tlx::make_counting<serving_request>(
        on_complete, this, buffer, offset, bytes, request::READ);

    disk_queues::get_instance()->add_request(req, get_queue_id());

    return req;
}

request_ptr disk_queued_file::awrite(
    void* buffer, offset_type offset, size_type bytes,
    const completion_handler& on_complete)
{
    request_ptr req = tlx::make_counting<serving_request>(
        on_complete, this, buffer, offset, bytes, request::WRITE);

    disk_queues::get_instance()->add_request(req, get_queue_id());

    return req;
}

} // namespace foxxll

/**************************************************************************/
