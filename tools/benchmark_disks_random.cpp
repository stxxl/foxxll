/***************************************************************************
 *  tools/benchmark_disks_random.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

/*
   example gnuplot command for the output of this program:
   (x-axis: offset in GiB, y-axis: bandwidth in MiB/s)

   plot \
        "disk.log" using ($2/1024):($7) w l title "read", \
        "disk.log" using ($2/1024):($4)  w l title "write"
 */

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <vector>

#include <tlx/logger.hpp>

#include <foxxll/io.hpp>
#include <foxxll/mng.hpp>
#include <tlx/cmdline_parser.hpp>

using foxxll::request_ptr;
using foxxll::timestamp;
using foxxll::external_size_type;

#define KiB (1024)
#define MiB (1024 * 1024)

template <typename AllocStrategy>
int run_test(external_size_type span, size_t block_size,
             external_size_type work_size, size_t batch_size,
             const std::string& optirw)
{
    bool do_init = (optirw.find('i') != std::string::npos);
    bool do_read = (optirw.find('r') != std::string::npos);
    bool do_write = (optirw.find('w') != std::string::npos);

    if (block_size > 128 * MiB) {
        LOG1 << "block_size " << block_size
             << " is larger than the limit 128 MiB.";
        return -1;
    }

    using block_type = foxxll::typed_block<128* MiB, size_t>;
    using BID_type = foxxll::BID<0>;

    size_t num_blocks = foxxll::div_ceil(work_size, block_size);
    size_t num_blocks_in_span = foxxll::div_ceil(span, block_size);

    num_blocks = std::min(num_blocks, num_blocks_in_span);
    if (num_blocks == 0) num_blocks = num_blocks_in_span;

    work_size = num_blocks * block_size;

    if (batch_size == 0)
        batch_size = work_size;

    size_t num_blocks_in_batch = foxxll::div_ceil(batch_size, block_size);

    size_t num_batches = foxxll::div_ceil(num_blocks, num_blocks_in_batch);

    block_type* buffer = new block_type;
    request_ptr* reqs = new request_ptr[num_blocks_in_span];
    std::vector<BID_type> blocks;

    // touch data, so it is actually allocated
    for (size_t i = 0; i < block_type::size; ++i)
        (*buffer)[i] = i;

    std::default_random_engine rng(std::random_device { } ());

    try {
        AllocStrategy alloc;

        blocks.resize(num_blocks_in_span);
        for (size_t i = 0; i < blocks.size(); ++i)
            blocks[i].size = block_size;

        foxxll::block_manager::get_instance()->new_blocks(
            alloc, blocks.begin(), blocks.end()
        );

        LOG1 << "# Span size: "
             << foxxll::add_IEC_binary_multiplier(span, "B") << " ("
             << num_blocks_in_span << " blocks of "
             << foxxll::add_IEC_binary_multiplier(block_size, "B") << ")";

        LOG1 << "# Work size: "
             << foxxll::add_IEC_binary_multiplier(work_size, "B") << " ("
             << num_blocks << " blocks of "
             << foxxll::add_IEC_binary_multiplier(block_size, "B") << ")";

        LOG1 << "# Batch size: "
             << foxxll::add_IEC_binary_multiplier(batch_size, "B") << " ("
             << batch_size << " blocks of "
             << foxxll::add_IEC_binary_multiplier(block_size, "B") << ")";

        LOG1 << "# Number of Batches: " << num_batches;

        double begin, end, elapsed;
        double time_write = 0, time_read = 0;

        if (do_init)
        {
            begin = timestamp();

            LOG1 << "First fill up space by writing sequentially...";
            for (size_t j = 0; j < num_blocks_in_span; j++)
                reqs[j] = blocks[j].write(buffer, block_size);
            wait_all(reqs, num_blocks_in_span);

            end = timestamp();
            elapsed = end - begin;
            LOG1 << "Written "
                 << std::setw(12) << num_blocks_in_span << " blocks in "
                 << std::fixed << std::setw(9) << std::setprecision(2)
                 << elapsed << " seconds: "
                 << std::setw(9) << std::setprecision(1)
                 << (static_cast<double>(num_blocks_in_span) / elapsed) << " blocks/s "
                 << std::setw(7) << std::setprecision(1)
                 << (static_cast<double>(num_blocks_in_span * block_size) / MiB / elapsed) << " MiB/s write ";
        }

        LOG1 << "Random block access...";

        std::shuffle(blocks.begin(), blocks.end(), rng);

        begin = timestamp();
        if (do_write)
        {
            size_t offset = 0, remaining_blocks = num_blocks;
            while (remaining_blocks) {
                size_t this_batch = std::min(num_blocks_in_batch, remaining_blocks);
                for (size_t j = 0; j < this_batch; j++)
                    reqs[j] = blocks[j + offset].write(buffer, block_size);
                wait_all(reqs, this_batch);
                offset += this_batch;
                remaining_blocks -= this_batch;
            }

            end = timestamp();
            time_write = elapsed = end - begin;

            LOG1 << "Written " << num_blocks << " blocks in "
                 << std::fixed << std::setw(5) << std::setprecision(2)
                 << elapsed << " seconds: "
                 << std::setw(5) << std::setprecision(1)
                 << (double(num_blocks) / elapsed) << " blocks/s "
                 << std::setw(5) << std::setprecision(1)
                 << (double(num_blocks * block_size) / MiB / elapsed) << " MiB/s write ";
        }

        std::shuffle(blocks.begin(), blocks.end(), rng);

        begin = timestamp();
        if (do_read)
        {
            size_t offset = 0, remaining_blocks = num_blocks;
            while (remaining_blocks) {
                size_t this_batch = std::min(num_blocks_in_batch, remaining_blocks);
                for (size_t j = 0; j < this_batch; j++)
                    reqs[j] = blocks[j + offset].read(buffer, block_size);
                wait_all(reqs, this_batch);
                offset += this_batch;
                remaining_blocks -= this_batch;
            }

            end = timestamp();
            time_read = elapsed = end - begin;

            LOG1 << "Read    " << num_blocks << " blocks in "
                 << std::fixed << std::setw(5) << std::setprecision(2)
                 << elapsed << " seconds: "
                 << std::setw(5) << std::setprecision(1)
                 << (double(num_blocks) / elapsed) << " blocks/s "
                 << std::setw(5) << std::setprecision(1)
                 << (double(num_blocks * block_size) / MiB / elapsed) << " MiB/s read";
        }

        std::cout << "RESULT"
                  << (getenv("RESULT") ? getenv("RESULT") : "")
                  << " span=" << span
                  << " work_size=" << work_size
                  << " op=" << (do_init ? "i" : "") << (do_read ? "r" : "") << (do_write ? "w" : "")
                  << " block_size=" << block_size
                  << " num_blocks=" << num_blocks
                  << " num_blocks_in_span=" << num_blocks_in_span
                  << " batch_size=" << batch_size
                  << " write_time=" << time_write
                  << " read_time=" << time_read
                  << " write_blocks_per_sec="
                  << (do_write ? num_blocks / time_write : 0)
                  << " read_blocks_per_sec="
                  << (do_read ? num_blocks / time_read : 0)
                  << " write_bytes_per_sec="
                  << (do_write ? (num_blocks * block_size) / time_write : 0)
                  << " read_bytes_per_sec="
                  << (do_read ? (num_blocks * block_size) / time_read : 0)
                  << std::endl;
    }
    catch (const std::exception& ex)
    {
        LOG1 << ex.what();
    }

    delete[] reqs;
    delete buffer;

    // foxxll::block_manager::get_instance()->delete_blocks(blocks.begin(), blocks.end());

    return 0;
}

int benchmark_disks_random(int argc, char* argv[])
{
    // parse command line

    tlx::CmdlineParser cp;

    external_size_type span, work_size = 0, batch_size = 0;
    external_size_type block_size = 8 * MiB;
    std::string optirw = "irw", allocstr;

    cp.add_param_bytes(
        "span", span,
        "Span of external memory to write/read to (e.g. 10GiB)."
    );
    cp.add_opt_param_bytes(
        "block_size", block_size,
        "Size of blocks to randomly write/read (default: 8MiB)."
    );
    cp.add_opt_param_bytes(
        "size", work_size,
        "Amount of data to operate on (e.g. 2GiB), default: whole span."
    );
    cp.add_opt_param_string(
        "i|r|w", optirw,
        "Operations: [i]nitialize, [r]ead, and/or [w]rite (default: all)."
    );
    cp.add_opt_param_string(
        "alloc", allocstr,
        "Block allocation strategy: random_cyclic, simple_random, "
        "fully_random, striping (default: random_cyclic)."
    );
    cp.add_bytes(
        'B', "batch_size", batch_size,
        "Amount of data in a batch (e.g. 2GiB), default: whole work size."
    );

    cp.set_description(
        "This program will benchmark _random_ block access on the disks "
        "configured by the standard .foxxll disk configuration files mechanism. "
        "Available block sizes are power of two from 4 KiB to 128 MiB. "
        "A set of three operations can be performed: sequential initialization, "
        "random reading and random writing."
    );

    if (!cp.process(argc, argv))
        return -1;

#define run_alloc(alloc) run_test<alloc>(span, block_size, work_size, batch_size, optirw)
    if (allocstr.size())
    {
        if (allocstr == "random_cyclic")
            return run_alloc(foxxll::random_cyclic);
        if (allocstr == "simple_random")
            return run_alloc(foxxll::simple_random);
        if (allocstr == "fully_random")
            return run_alloc(foxxll::fully_random);
        if (allocstr == "striping")
            return run_alloc(foxxll::striping);

        LOG1 << "Unknown allocation strategy '" << allocstr << "'";
        cp.print_usage();
        return -1;
    }

    return run_alloc(foxxll::default_alloc_strategy);
#undef run_alloc
}

/**************************************************************************/
