/***************************************************************************
 *  foxxll/io/request.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013-2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_IO_REQUEST_HEADER
#define FOXXLL_IO_REQUEST_HEADER

#include <cassert>
#include <functional>
#include <memory>
#include <string>

#include <tlx/counting_ptr.hpp>
#include <tlx/delegate.hpp>

#include <foxxll/common/exceptions.hpp>
#include <foxxll/io/request_interface.hpp>

namespace foxxll {

//! \addtogroup reqlayer
//! \{

constexpr size_t BlockAlignment = 4096;

class file;
class request;

//! A reference counting pointer for \c file.
using file_ptr = tlx::counting_ptr<file>;

//! A reference counting pointer for \c request.
using request_ptr = tlx::counting_ptr<request>;

//! completion handler
using completion_handler = tlx::delegate<void(request* r, bool success)>;

//! Request object encapsulating basic properties like file and offset.
class request : virtual public request_interface, public tlx::reference_counter
{
    constexpr static bool debug = false;
    friend class linuxaio_queue;

protected:
    completion_handler on_complete_;
    std::unique_ptr<io_error> error_;

protected:
    //! \name Base Parameter of an I/O Request
    //! \{

    //! file implementation to perform I/O with
    file* file_;
    //! data buffer to transfer
    void* buffer_;
    //! offset within file
    offset_type offset_;
    //! number of bytes at buffer_ to transfer
    size_type bytes_;
    //! READ or WRITE
    read_or_write op_;

    //! \}

public:
    request(const completion_handler& on_complete,
            file* file, void* buffer, offset_type offset, size_type bytes,
            read_or_write op);

    //! non-copyable: delete copy-constructor
    request(const request&) = delete;
    //! non-copyable: delete assignment operator
    request& operator = (const request&) = delete;
    //! move-constructor: default
    request(request&&) = default;
    //! move-assignment operator: default
    request& operator = (request&&) = default;

    virtual ~request();

public:
    //! \name Accessors
    //! \{

    file * get_file() const { return file_; }
    void * buffer() const { return buffer_; }
    offset_type offset() const { return offset_; }
    size_type bytes() const { return bytes_; }
    read_or_write op() const { return op_; }

    void check_alignment() const;

    std::ostream & print(std::ostream& out) const final;

    //! Inform the request object that an error occurred during the I/O
    //! execution.
    void error_occured(const char* msg);

    //! Inform the request object that an error occurred during the I/O
    //! execution.
    void error_occured(const std::string& msg);

    //! Rises an exception if there were error with the I/O.
    void check_errors()
    {
        if (error_.get())
            throw *(error_.get());
    }

    const char * io_type() const override;

    //! \}

protected:
    void check_nref(bool after = false)
    {
        if (reference_count() < 2)
            check_nref_failed(after);
    }

private:
    void check_nref_failed(bool after);
};

std::ostream& operator << (std::ostream& out, const request& req);

//! \}

} // namespace foxxll

#endif // !FOXXLL_IO_REQUEST_HEADER
// vim: et:ts=4:sw=4
