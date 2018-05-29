/***************************************************************************
 *  foxxll/io/request_with_waiters.hpp
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

#ifndef FOXXLL_IO_REQUEST_WITH_WAITERS_HEADER
#define FOXXLL_IO_REQUEST_WITH_WAITERS_HEADER

#include <mutex>
#include <set>

#include <foxxll/common/onoff_switch.hpp>
#include <foxxll/io/request.hpp>

namespace foxxll {

//! \addtogroup foxxll_reqlayer
//! \{

//! Request that is aware of threads waiting for it to complete.
class request_with_waiters : public request
{
    std::mutex waiters_mutex_;
    std::set<onoff_switch*> waiters_;

protected:
    bool add_waiter(onoff_switch* sw) final;
    void delete_waiter(onoff_switch* sw) final;
    void notify_waiters() final;

    //! returns number of waiters
    size_t num_waiters();

public:
    request_with_waiters(
        const completion_handler& on_complete,
        file* file, void* buffer, offset_type offset, size_type bytes,
        read_or_write op)
        : request(on_complete, file, buffer, offset, bytes, op)
    { }
};

//! \}

} // namespace foxxll

#endif // !FOXXLL_IO_REQUEST_WITH_WAITERS_HEADER

/**************************************************************************/
