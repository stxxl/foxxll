/***************************************************************************
 *  tests/io/test_io.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cstring>
#include <limits>

#include <foxxll/common/aligned_alloc.hpp>
#include <foxxll/io.hpp>

//! \example io/test_io.cpp
//! This is an example of use of \c \<foxxll\> files, requests, and
//! completion tracking mechanisms, i.e. \c foxxll::file , \c foxxll::request

using foxxll::file;

struct my_handler
{
    void operator () (foxxll::request* ptr, bool success)
    {
        STXXL_MSG("Request completed: " << ptr << " success: " << success);
    }
};

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " tempdir" << std::endl;
        return -1;
    }

    std::string tempfilename[2];
    tempfilename[0] = std::string(argv[1]) + "/test_io_1.dat";
    tempfilename[1] = std::string(argv[1]) + "/test_io_2.dat";

    std::cout << sizeof(void*) << std::endl;
    const int size = 1024 * 384;
    char* buffer = (char*)foxxll::aligned_alloc<4096>(size);
    memset(buffer, 0, size);

#if STXXL_HAVE_MMAP_FILE
    foxxll::file_ptr file1 = tlx::make_counting<foxxll::mmap_file>(
        tempfilename[0], file::CREAT | file::RDWR | file::DIRECT, 0);
    file1->set_size(size * 1024);
#endif

    foxxll::file_ptr file2 = tlx::make_counting<foxxll::syscall_file>(
        tempfilename[1], file::CREAT | file::RDWR | file::DIRECT, 1);

    foxxll::request_ptr req[16];
    unsigned i;
    for (i = 0; i < 16; i++)
        req[i] = file2->awrite(buffer, i * size, size, my_handler());

    wait_all(req, 16);

    // check behaviour of having requests to the same location at the same time
    for (i = 2; i < 16; i++)
        req[i] = file2->awrite(buffer, 0, size, my_handler());
    req[0] = file2->aread(buffer, 0, size, my_handler());
    req[1] = file2->awrite(buffer, 0, size, my_handler());

    wait_all(req, 16);

    foxxll::aligned_dealloc<4096>(buffer);

    std::cout << *(foxxll::stats::get_instance());

    size_t sz;
    for (sz = 123, i = 0; i < 20; ++i, sz *= 10)
        STXXL_MSG(">>>" << foxxll::add_SI_multiplier(sz) << "<<<");
    for (sz = 123, i = 0; i < 20; ++i, sz *= 10)
        STXXL_MSG(">>>" << foxxll::add_SI_multiplier(sz, "B") << "<<<");
    STXXL_MSG(">>>" << foxxll::add_SI_multiplier(std::numeric_limits<uint64_t>::max(), "B") << "<<<");
    for (sz = 123, i = 0; i < 20; ++i, sz *= 10)
        STXXL_MSG(">>>" << foxxll::add_IEC_binary_multiplier(sz) << "<<<");
    for (sz = 123, i = 0; i < 20; ++i, sz *= 10)
        STXXL_MSG(">>>" << foxxll::add_IEC_binary_multiplier(sz, "B") << "<<<");
    STXXL_MSG(">>>" << foxxll::add_IEC_binary_multiplier(std::numeric_limits<uint64_t>::max(), "B") << "<<<");

#if STXXL_HAVE_MMAP_FILE
    file1->close_remove();
#endif

    file2->close_remove();

    return 0;
}
