/***************************************************************************
 *  foxxll/io/request_target.hpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2017 Manuel Penschuck <manuel@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file_ LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifdef STXXL_IO_REQUEST_TARGET_HEADER
#define STXXL_IO_REQUEST_TARGET_HEADER

#include <foxxll/io/request.hpp>

namespace foxxll {

/*!
 * This container extracts only those information from foxxll::request
 * to be able to tell whether two requests target overlapping areas.
 * After the constructor was called, it will never access memory foreign
 * memory locations and hence can be used w/o interfering with ref counters (etc.)
 */
class request_target {
public:
    request_target() = delete;

    explicit request_target(const request& req) noexcept
        : file_(req.get_file())
        , offset_(req.get_offset())
        , bytes_(req.get_size())
        , op_(req.get_op())
    {}

    request::read_or_write op() const {return op_;}

    bool do_overlap(const request_target& o) const {
        return (file_ == o.file_) && (
               ((offset_ <= o.offset_) && (o.offset_ < (offset_ + bytes_)))
            || ((o.offset_ <= offset_) && (offset_ < (o.offset_ + o.bytes_))));
    }

    bool contains(const request_target& o) const {
        return (file_ == o.file_)
            && (offset_ <= o.offset_)
            && ((offset_ + bytes_) >= (o.offset_ + o.bytes_));
    }

private:
    foxxll::file_* file_;
    request::offset_type offset_;
    request::size_type bytes_;
    request::read_or_write op_;
};
} // foxxll

#endif // STXXL_IO_REQUEST_TARGET_HEADER