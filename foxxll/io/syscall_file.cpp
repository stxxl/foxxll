/***************************************************************************
 *  foxxll/io/syscall_file.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <limits>
#include <mutex>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/config.hpp>
#include <foxxll/io/iostats.hpp>
#include <foxxll/io/request.hpp>
#include <foxxll/io/request_interface.hpp>
#include <foxxll/io/syscall_file.hpp>
#include <foxxll/io/ufs_platform.hpp>

namespace foxxll {

void syscall_file::serve(void* buffer, offset_type offset, size_type bytes,
                         request::read_or_write op)
{
    std::unique_lock<std::mutex> fd_lock(fd_mutex_);

    auto* cbuffer = static_cast<char*>(buffer);

    file_stats::scoped_read_write_timer read_write_timer(
        file_stats_, bytes, op == request::WRITE);

    while (bytes > 0)
    {
        off_t rc = ::lseek(file_des_, offset, SEEK_SET);
        if (rc < 0)
        {
            FOXXLL_THROW_ERRNO
                (io_error,
                " this=" << this <<
                " call=::lseek(fd,offset,SEEK_SET)" <<
                " path=" << filename_ <<
                " fd=" << file_des_ <<
                " offset=" << offset <<
                " buffer=" << cbuffer <<
                " bytes=" << bytes <<
                " op=" << ((op == request::READ) ? "READ" : "WRITE") <<
                " rc=" << rc);
        }

        if (op == request::READ)
        {
#if FOXXLL_MSVC
            assert(bytes <= std::numeric_limits<unsigned int>::max());
            if ((rc = ::read(file_des_, cbuffer, (unsigned int)bytes)) <= 0)
#else
            if ((rc = ::read(file_des_, cbuffer, bytes)) <= 0)
#endif
            {
                FOXXLL_THROW_ERRNO
                    (io_error,
                    " this=" << this <<
                    " call=::read(fd,buffer,bytes)" <<
                    " path=" << filename_ <<
                    " fd=" << file_des_ <<
                    " offset=" << offset <<
                    " buffer=" << buffer <<
                    " bytes=" << bytes <<
                    " op=" << "READ" <<
                    " rc=" << rc);
            }
            bytes = static_cast<size_type>(bytes - rc);
            offset += rc;
            cbuffer += rc;

            if (bytes > 0 && offset == this->_size())
            {
                // read request extends past end-of-file
                // fill reminder with zeroes
                memset(cbuffer, 0, bytes);
                bytes = 0;
            }
        }
        else
        {
#if FOXXLL_MSVC
            assert(bytes <= std::numeric_limits<unsigned int>::max());
            if ((rc = ::write(file_des_, cbuffer, (unsigned int)bytes)) <= 0)
#else
            if ((rc = ::write(file_des_, cbuffer, bytes)) <= 0)
#endif
            {
                FOXXLL_THROW_ERRNO
                    (io_error,
                    " this=" << this <<
                    " call=::write(fd,buffer,bytes)" <<
                    " path=" << filename_ <<
                    " fd=" << file_des_ <<
                    " offset=" << offset <<
                    " buffer=" << buffer <<
                    " bytes=" << bytes <<
                    " op=" << "WRITE" <<
                    " rc=" << rc);
            }
            bytes = static_cast<size_type>(bytes - rc);
            offset += rc;
            cbuffer += rc;
        }
    }
}

const char* syscall_file::io_type() const
{
    return "syscall";
}

} // namespace foxxll
// vim: et:ts=4:sw=4
