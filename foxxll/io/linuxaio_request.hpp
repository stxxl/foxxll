/***************************************************************************
 *  foxxll/io/linuxaio_request.hpp
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

#ifndef FOXXLL_IO_LINUXAIO_REQUEST_HEADER
#define FOXXLL_IO_LINUXAIO_REQUEST_HEADER

#include <foxxll/io/linuxaio_file.hpp>

#if FOXXLL_HAVE_LINUXAIO_FILE

#include <linux/aio_abi.h>

#include <tlx/logger.hpp>

#include <foxxll/io/request_with_state.hpp>

namespace foxxll {

//! \addtogroup reqlayer
//! \{

// forward declarations
class linuxaio_queue;

//! Request for an linuxaio_file.
class linuxaio_request : public request_with_state
{
    constexpr static bool debug = false;

    template <class base_file_type>
    friend class fileperblock_file;

    //! control block of async request
    iocb cb_;
    double time_posted_;

    void fill_control_block();

public:
    linuxaio_request(
        const completion_handler& on_complete,
        file* file, void* buffer, offset_type offset, size_type bytes,
        const read_or_write& op)
        : request_with_state(on_complete, file, buffer, offset, bytes, op)
    {
        assert(dynamic_cast<linuxaio_file*>(file));
        LOG << "linuxaio_request[" << this << "]" <<
            " linuxaio_request" <<
            "(file=" << file << " buffer=" << buffer <<
            " offset=" << offset << " bytes=" << bytes <<
            " op=" << op << ")";
    }

    bool post();
    bool cancel() final;
    bool cancel_aio(linuxaio_queue* queue);
    void completed(bool posted, bool canceled);
    void completed(bool canceled) { completed(true, canceled); }
};

//! \}

} // namespace foxxll

#endif // #if FOXXLL_HAVE_LINUXAIO_FILE

#endif // !FOXXLL_IO_LINUXAIO_REQUEST_HEADER

/**************************************************************************/
