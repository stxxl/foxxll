/***************************************************************************
 *  foxxll/io/serving_request.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_IO_SERVING_REQUEST_HEADER
#define FOXXLL_IO_SERVING_REQUEST_HEADER

#include <foxxll/io/request_with_state.hpp>

namespace foxxll {

//! \addtogroup foxxll_reqlayer
//! \{

//! Request which serves an I/O by calling the synchronous routine of the file.
class serving_request : public request_with_state
{
    constexpr static bool debug = false;

    template <class base_file_type>
    friend class fileperblock_file;

    friend class request_queue_impl_qwqr;
    friend class request_queue_impl_1q;

public:
    serving_request(
        const completion_handler& on_complete,
        file* file, void* buffer, offset_type offset, size_type bytes,
        read_or_write op);

protected:
    virtual void serve();

public:
    const char * io_type() const final;
};

//! \}

} // namespace foxxll

#endif // !FOXXLL_IO_SERVING_REQUEST_HEADER

/**************************************************************************/
