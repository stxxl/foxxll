/***************************************************************************
 *  foxxll/io/request_with_state.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_IO_REQUEST_WITH_STATE_HEADER
#define FOXXLL_IO_REQUEST_WITH_STATE_HEADER

#include <foxxll/common/shared_state.hpp>
#include <foxxll/io/request.hpp>
#include <foxxll/io/request_with_waiters.hpp>

namespace foxxll {

//! \addtogroup reqlayer
//! \{

//! Request with completion shared_state.
class request_with_state : public request_with_waiters
{
    constexpr static bool debug = false;

protected:
    //! states of request.
    //! OP - operating, DONE - request served, READY2DIE - can be destroyed
    enum request_state { OP = 0, DONE = 1, READY2DIE = 2 };

    shared_state<request_state> state_;

protected:
    request_with_state(
        const completion_handler& on_complete,
        file* file, void* buffer, offset_type offset, size_type bytes,
        read_or_write op)
        : request_with_waiters(on_complete, file, buffer, offset, bytes, op),
          state_(OP)
    { }

public:
    virtual ~request_with_state();
    void wait(bool measure_time = true) final;
    bool poll() final;
    bool cancel() override;

protected:
    void completed(bool canceled) override;
};

//! \}

} // namespace foxxll

#endif // !FOXXLL_IO_REQUEST_WITH_STATE_HEADER
