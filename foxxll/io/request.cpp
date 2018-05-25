/***************************************************************************
 *  foxxll/io/request.cpp
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

#include <ostream>

#include <tlx/logger.hpp>

#include <foxxll/io/file.hpp>
#include <foxxll/io/request.hpp>

namespace foxxll {

request::request(
    const completion_handler& on_complete,
    file* file, void* buffer, offset_type offset, size_type bytes,
    read_or_write op)
    : on_complete_(on_complete),
      file_(file), buffer_(buffer), offset_(offset), bytes_(bytes),
      op_(op)
{
    LOG << "request_with_state[" << static_cast<void*>(this) << "]::request(...), ref_cnt=" << reference_count();
    file_->add_request_ref();
}

request::~request()
{
    LOG << "request_with_state[" << static_cast<void*>(this) << "]::~request(), ref_cnt=" << reference_count();
}

void request::check_alignment() const
{
    if (offset_ % BlockAlignment != 0)
        LOG1 << "Offset is not aligned: modulo " <<
            BlockAlignment << " = " << offset_ % BlockAlignment;

    if (bytes_ % BlockAlignment != 0)
        LOG1 << "Size is not a multiple of " <<
            BlockAlignment << ", = " << bytes_ % BlockAlignment;

    if (size_t(buffer_) % BlockAlignment != 0)
        LOG1 << "Buffer is not aligned: modulo " <<
            BlockAlignment << " = " << size_t(buffer_) % BlockAlignment <<
            " (" << buffer_ << ")";
}

void request::check_nref_failed(bool after)
{
    LOG1 << "WARNING: serious error, reference to the request is lost " <<
    (after ? "after" : "before") << " serve()" <<
        " nref=" << reference_count() <<
        " this=" << this <<
        " offset=" << offset_ <<
        " buffer=" << buffer_ <<
        " bytes=" << bytes_ <<
        " op=" << ((op_ == READ) ? "READ" : "WRITE") <<
        " file=" << file_ <<
        " iotype=" << file_->io_type();
}

std::ostream& request::print(std::ostream& out) const
{
    out << "File object address: " << file_
        << " Buffer address: " << static_cast<void*>(buffer_)
        << " File offset: " << offset_
        << " Transfer size: " << bytes_ << " bytes"
        " Type of transfer: " << ((op_ == READ) ? "READ" : "WRITE");
    return out;
}

void request::error_occured(const char* msg)
{
    error_.reset(new io_error(msg));
}

void request::error_occured(const std::string& msg)
{
    error_.reset(new io_error(msg));
}

const char* request::io_type() const
{
    return file_->io_type();
}

void request::release_file_reference()
{
    if (file_) {
        file_->delete_request_ref();
        file_ = nullptr;
    }
}

std::ostream& operator << (std::ostream& out, const request& req)
{
    return req.print(out);
}

} // namespace foxxll

/**************************************************************************/
