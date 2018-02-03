/***************************************************************************
 *  tests/io/test_cancel.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2009-2011 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cstring>

#include <foxxll/common/aligned_alloc.hpp>
#include <foxxll/io.hpp>

//! \example io/test_cancel.cpp
//! This tests the request cancelation mechanisms.

using foxxll::file;

struct print_completion
{
    void operator () (foxxll::request* ptr, bool success)
    {
        std::cout << "Request completed: " << ptr
                  << " success: " << success << std::endl;
    }
};

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " filetype tempfile" << std::endl;
        return -1;
    }

    constexpr size_t size = 16 * 1024 * 1024;
    constexpr size_t kNumBlocks = 16;

    auto* buffer = static_cast<char*>(foxxll::aligned_alloc<4096>(size));
    memset(buffer, 0, size);

    foxxll::file_ptr file = foxxll::create_file(
            argv[1], argv[2],
            foxxll::file::CREAT | foxxll::file::RDWR | foxxll::file::DIRECT
        );

    file->set_size(kNumBlocks * size);
    foxxll::request_ptr req[kNumBlocks];

    // without cancelation
    std::cout << "Posting " << kNumBlocks << " requests." << std::endl;
    foxxll::stats_data stats1(*foxxll::stats::get_instance());
    for (unsigned i = 0; i < kNumBlocks; i++)
        req[i] = file->awrite(buffer, i * size, size, print_completion());
    wait_all(req, kNumBlocks);
    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats1;

    // with cancelation
    std::cout << "Posting " << kNumBlocks << " requests." << std::endl;
    foxxll::stats_data stats2(*foxxll::stats::get_instance());
    for (unsigned i = 0; i < kNumBlocks; i++)
        req[i] = file->awrite(buffer, i * size, size, print_completion());

    // cancel first half
    std::cout << "Canceling first " << kNumBlocks / 2 << " requests." << std::endl;
    size_t num_canceled = cancel_all(req, req + kNumBlocks / 2);
    std::cout << "Successfully canceled " << num_canceled << " requests." << std::endl;

    // cancel every second in second half
    for (unsigned i = kNumBlocks / 2; i < kNumBlocks; i += 2)
    {
        std::cout << "Canceling request " << &(*(req[i])) << std::endl;
        if (req[i]->cancel())
            std::cout << "Request canceled: " << &(*(req[i])) << std::endl;
        else
            std::cout << "Request not canceled: " << &(*(req[i])) << std::endl;
    }
    wait_all(req, kNumBlocks);
    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats2;

    foxxll::aligned_dealloc<4096>(buffer);

    file->close_remove();

    return 0;
}
