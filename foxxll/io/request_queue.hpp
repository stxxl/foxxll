/***************************************************************************
 *  foxxll/io/request_queue.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2011 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_IO_REQUEST_QUEUE_HEADER
#define FOXXLL_IO_REQUEST_QUEUE_HEADER

#include <tlx/unused.hpp>

#include <foxxll/io/request.hpp>

namespace foxxll {

//! \addtogroup foxxll_reqlayer
//! \{

//! Interface of a request_queue to which requests can be added and canceled.
class request_queue
{
public:
    enum priority_op { READ, WRITE, NONE };

public:
    request_queue() = default;

    //! non-copyable: delete copy-constructor
    request_queue(const request_queue&) = delete;
    //! non-copyable: delete assignment operator
    request_queue& operator = (const request_queue&) = delete;
    //! move-constructor: default
    request_queue(request_queue&&) = default;
    //! move-assignment operator: default
    request_queue& operator = (request_queue&&) = default;

public:
    virtual void add_request(request_ptr& req) = 0;
    virtual bool cancel_request(request_ptr& req) = 0;
    virtual ~request_queue() { }
    virtual void set_priority_op(const priority_op& p) { tlx::unused(p); }
};

//! \}

} // namespace foxxll

#endif // !FOXXLL_IO_REQUEST_QUEUE_HEADER

/**************************************************************************/
