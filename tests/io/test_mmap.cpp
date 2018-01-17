/***************************************************************************
 *  tests/io/test_mmap.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2007 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <tlx/logger.hpp>

#include <foxxll/common/aligned_alloc.hpp>
#include <foxxll/io.hpp>

struct my_handler
{
    void operator () (foxxll::request* ptr, bool /* success */)
    {
        LOG1 << "Request completed: " << ptr;
    }
};

void testIO()
{
    const int size = 1024 * 384;
    char* buffer = static_cast<char*>(foxxll::aligned_alloc<STXXL_BLOCK_ALIGN>(size));
    memset(buffer, 0, size);
#if FOXXLL_WINDOWS
    const char* paths[2] = { "data1", "data2" };
#else
    const char* paths[2] = { "/var/tmp/data1", "/var/tmp/data2" };
    foxxll::file_ptr file1 = tlx::make_counting<foxxll::mmap_file>(
        paths[0], foxxll::file::CREAT | foxxll::file::RDWR, 0);
    file1->set_size(size * 1024);
#endif

    foxxll::file_ptr file2 = tlx::make_counting<foxxll::syscall_file>(
        paths[1], foxxll::file::CREAT | foxxll::file::RDWR, 1);

    foxxll::request_ptr req[16];
    unsigned i = 0;
    for ( ; i < 16; i++)
        req[i] = file2->awrite(buffer, i * size, size, my_handler());

    foxxll::wait_all(req, 16);

    foxxll::aligned_dealloc<STXXL_BLOCK_ALIGN>(buffer);

#if !FOXXLL_WINDOWS
    file1->close_remove();
#endif
    file2->close_remove();
}

void testIOException()
{
    foxxll::file::unlink("TestFile");
    // try to open non-existing files
    FOXXLL_CHECK_THROW(foxxll::mmap_file file1("TestFile", foxxll::file::RDWR, 0), foxxll::io_error);
    FOXXLL_CHECK_THROW(foxxll::syscall_file file1("TestFile", foxxll::file::RDWR, 0), foxxll::io_error);
}

int main()
{
    testIO();
    testIOException();
}
