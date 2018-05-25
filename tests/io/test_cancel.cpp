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

#include <tlx/logger.hpp>

#include <foxxll/common/aligned_alloc.hpp>
#include <foxxll/io.hpp>

//! \example io/test_cancel.cpp
//! This tests the request cancelation mechanisms.

using foxxll::file;

struct print_completion
{
    void operator () (foxxll::request* ptr, bool success)
    {
        LOG1 << "Request completed: " << ptr << " success: " << success;
    }
};

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        LOG1 << "Usage: " << argv[0] << " filetype tempfile";
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
    LOG1 << "Posting " << kNumBlocks << " requests.";
    {
        foxxll::scoped_print_iostats stats("Posting");
        for (unsigned i = 0; i < kNumBlocks; i++)
            req[i] = file->awrite(buffer, i * size, size, print_completion());
        wait_all(req, kNumBlocks);
    }

    // with cancelation
    foxxll::stats_data stats2(*foxxll::stats::get_instance());
    {
        foxxll::scoped_print_iostats stats("Posting with cancellation");
        for (unsigned i = 0; i < kNumBlocks; i++)
            req[i] = file->awrite(buffer, i * size, size, print_completion());

        // cancel first half
        LOG1 << "Canceling first " << (kNumBlocks / 2) << " requests.";
        size_t num_canceled = cancel_all(req, req + kNumBlocks / 2);
        LOG1 << "Successfully canceled " << num_canceled << " requests.";

        // cancel every second in second half
        for (unsigned i = kNumBlocks / 2; i < kNumBlocks; i += 2) {
            LOG1 << "Canceling request " << &(*(req[i]));
            if (req[i]->cancel())
                LOG1 << "Request canceled: " << &(*(req[i]));
            else
                LOG1 << "Request not canceled: " << &(*(req[i]));
        }
        wait_all(req, kNumBlocks);
    }

    foxxll::aligned_dealloc<4096>(buffer);

    file->close_remove();

    return 0;
}

/**************************************************************************/
