/***************************************************************************
 *  foxxll/io/disk_queued_file.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_IO_DISK_QUEUED_FILE_HEADER
#define FOXXLL_IO_DISK_QUEUED_FILE_HEADER

#include <foxxll/io/file.hpp>
#include <foxxll/io/request.hpp>

namespace foxxll {

//! \addtogroup foxxll_fileimpl
//! \{

//! Implementation of some file methods based on serving_request.
class disk_queued_file : public virtual file
{
    int queue_id_, allocator_id_;

public:
    disk_queued_file(int queue_id, int allocator_id)
        : queue_id_(queue_id), allocator_id_(allocator_id)
    { }

    request_ptr aread(
        void* buffer, offset_type pos, size_type bytes,
        const completion_handler& on_complete = completion_handler()) override;

    request_ptr awrite(
        void* buffer, offset_type pos, size_type bytes,
        const completion_handler& on_complete = completion_handler()) override;

    int get_queue_id() const override
    {
        return queue_id_;
    }

    int get_allocator_id() const override
    {
        return allocator_id_;
    }
};

//! \}

} // namespace foxxll

#endif // !FOXXLL_IO_DISK_QUEUED_FILE_HEADER

/**************************************************************************/
