/***************************************************************************
 *  foxxll/io/mmap_file.cpp
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

#include <foxxll/io/mmap_file.hpp>

#if FOXXLL_HAVE_MMAP_FILE

#include <sys/mman.h>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/io/iostats.hpp>
#include <foxxll/io/ufs_platform.hpp>

namespace foxxll {

void mmap_file::serve(void* buffer, offset_type offset, size_type bytes,
                      request::read_or_write op)
{
    std::unique_lock<std::mutex> fd_lock(fd_mutex_);

    //assert(offset + bytes <= _size());

    file_stats::scoped_read_write_timer read_write_timer(
        file_stats_, bytes, op == request::WRITE);

    int prot = (op == request::READ) ? PROT_READ : PROT_WRITE;
    void* mem = mmap(nullptr, bytes, prot, MAP_SHARED, file_des_, offset);

    if (mem == MAP_FAILED)
    {
        FOXXLL_THROW_ERRNO(
            io_error,
            " mmap() failed." <<
                " path=" << filename_ <<
                " bytes=" << bytes <<
                " Page size: " << sysconf(_SC_PAGESIZE) <<
                " offset modulo page size " << (offset % sysconf(_SC_PAGESIZE))
        );
    }
    else if (mem == 0)
    {
        FOXXLL_THROW_ERRNO(io_error, "mmap() returned nullptr");
    }
    else
    {
        if (op == request::READ)
        {
            memcpy(buffer, mem, bytes);
        }
        else
        {
            memcpy(mem, buffer, bytes);
        }
        FOXXLL_THROW_ERRNO_NE_0(
            munmap(mem, bytes), io_error,
            "munmap() failed"
        );
    }
}

const char* mmap_file::io_type() const
{
    return "mmap";
}

} // namespace foxxll

#endif // #if FOXXLL_HAVE_MMAP_FILE

/**************************************************************************/
