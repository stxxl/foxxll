/***************************************************************************
 *  tests/io/test_io_sizes.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/io.hpp>
#include <foxxll/mng.hpp>

//! \example io/test_io_sizes.cpp
//! This tests the maximum chunk size that a file type can handle with a single request.

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        LOG1 << "Usage: " << argv[0] << " filetype tempfile maxsize";
        return -1;
    }

    size_t max_size = atoi(argv[3]);
    auto* buffer = static_cast<size_t*>(foxxll::aligned_alloc<4096>(max_size));

    try
    {
        foxxll::file_ptr file = foxxll::create_file(
            argv[1], argv[2],
            foxxll::file::CREAT | foxxll::file::RDWR | foxxll::file::DIRECT);
        file->set_size(max_size);

        foxxll::request_ptr req;

        {
            foxxll::scoped_print_iostats stats("IO");
            for (size_t size = 4096; size < max_size; size *= 2) {
                //generate data
                for (size_t i = 0; i < size / sizeof(size_t); ++i)
                    buffer[i] = i;

                //write
                LOG1 << tlx::format_iec_units(size) << "are being written at once";
                req = file->awrite(buffer, 0, size);
                wait_all(&req, 1);

                //fill with wrong data
                for (size_t i = 0; i < size / sizeof(size_t); ++i)
                    buffer[i] = 0xFFFFFFFFFFFFFFFFull;

                //read again
                LOG1 << tlx::format_iec_units(size) << "are being read at once";
                req = file->aread(buffer, 0, size);
                wait_all(&req, 1);

                //check
                bool wrong = false;
                for (size_t i = 0; i < size / sizeof(size_t); ++i)
                    if (buffer[i] != i) {
                        LOG1 << "Read inconsistent data at position " << i * sizeof(size_t);
                        wrong = true;
                        break;
                    }

                if (wrong)
                    break;
            }
        }

        file->close_remove();
    }
    catch (foxxll::io_error e)
    {
        die(e.what());
    }

    foxxll::aligned_dealloc<4096>(buffer);

    return 0;
}

/**************************************************************************/
