/***************************************************************************
 *  tools/benchmark_disks.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

/*
  This programm will benchmark the disks configured via .foxxll disk
  configuration files. The block manager is used to read and write blocks using
  the different allocation strategies.
*/

/*
   example gnuplot command for the output of this program:
   (x-axis: offset in GiB, y-axis: bandwidth in MiB/s)

   plot \
        "disk.log" using ($2/1024):($7) w l title "read", \
        "disk.log" using ($2/1024):($4)  w l title "write"
 */

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <tlx/cmdline_parser.hpp>
#include <tlx/logger.hpp>

#include <foxxll/io.hpp>
#include <foxxll/mng.hpp>

using foxxll::timestamp;
using foxxll::external_size_type;

#ifdef BLOCK_ALIGN
 #undef BLOCK_ALIGN
#endif

#define BLOCK_ALIGN  4096

#define POLL_DELAY 1000

#define CHECK_AFTER_READ 0

#define KiB (1024)
#define MiB (1024 * 1024)

bool g_quiet = false;
double g_time_limit = NAN;

template <typename AllocStrategy>
int benchmark_disks_alloc(
    external_size_type size, external_size_type start_offset,
    size_t batch_size, const size_t raw_block_size,
    const std::string& optrw)
{
    external_size_type endpos = start_offset + size;

    if (size == 0)
        endpos = std::numeric_limits<external_size_type>::max();

    bool do_read = (optrw.find('r') != std::string::npos);
    bool do_write = (optrw.find('w') != std::string::npos);

    // initialize disk configuration
    foxxll::block_manager::get_instance();

    // construct block type

    const size_t block_size = raw_block_size / sizeof(uint32_t);

    using block_type = uint32_t *;
    using BID = foxxll::BID<0>;

    if (batch_size == 0)
        batch_size = foxxll::config::get_instance()->disks_number();

    // calculate total bytes processed in a batch
    batch_size = raw_block_size * batch_size;

    size_t num_blocks_per_batch = foxxll::div_ceil(batch_size, raw_block_size);
    batch_size = num_blocks_per_batch * raw_block_size;

    std::vector<block_type> buffer(num_blocks_per_batch);
    foxxll::request_ptr* reqs = new foxxll::request_ptr[num_blocks_per_batch];
    std::vector<BID> bids;
    double total_time_read = 0, total_time_write = 0;
    external_size_type total_size_read = 0, total_size_write = 0;

    LOG1 << "# Batch size: "
         << foxxll::add_IEC_binary_multiplier(batch_size, "B") << " ("
         << num_blocks_per_batch << " blocks of "
         << foxxll::add_IEC_binary_multiplier(raw_block_size, "B") << ")"
         << " using " << AllocStrategy().name();

    // allocate data blocks
    for (size_t j = 0; j < num_blocks_per_batch; ++j) {
        buffer[j] = reinterpret_cast<block_type>(
                foxxll::aligned_alloc<4096>(raw_block_size));
    }

    // touch data, so it is actually allocated
    for (size_t j = 0; j < num_blocks_per_batch; ++j) {
        for (size_t i = 0; i < block_size; ++i)
            buffer[j][i] = static_cast<uint32_t>(j * block_size + i);
    }

    try {
        AllocStrategy alloc;
        size_t current_batch_size;

        double ts_begin = timestamp();

        for (external_size_type offset = 0; offset < endpos; offset += current_batch_size)
        {
            std::stringstream ss;

            current_batch_size = static_cast<size_t>(
                    std::min<external_size_type>(batch_size, endpos - offset));
#if CHECK_AFTER_READ
            const size_t current_batch_size_int = current_batch_size / sizeof(uint32_t);
#endif
            const size_t current_num_blocks_per_batch =
                foxxll::div_ceil(current_batch_size, raw_block_size);

            size_t num_total_blocks = bids.size();
            bids.resize(num_total_blocks + current_num_blocks_per_batch);

            // fill in block size of BID<0> variable blocks
            for (BID& b : bids) b.size = raw_block_size;

            foxxll::block_manager::get_instance()->new_blocks(
                alloc, bids.begin() + num_total_blocks, bids.end()
            );

            if (offset < start_offset)
                continue;

            if (!g_quiet)
                ss << "Offset    " << std::setw(7) << (offset / MiB) << " MiB: "
                   << std::fixed;

            double begin = timestamp(), end, elapsed;

            if (do_write)
            {
                for (size_t j = 0; j < current_num_blocks_per_batch; j++)
                    reqs[j] = bids[num_total_blocks + j].write(buffer[j], raw_block_size);

                wait_all(reqs, current_num_blocks_per_batch);

                end = timestamp();
                elapsed = end - begin;
                total_size_write += current_batch_size;
                total_time_write += elapsed;
            }
            else
                elapsed = 0.0;

            if (!g_quiet)
                ss << std::setw(5) << std::setprecision(1)
                   << (double(current_batch_size) / MiB / elapsed)
                   << " MiB/s write, ";

            begin = timestamp();

            if (do_read)
            {
                for (size_t j = 0; j < current_num_blocks_per_batch; j++)
                    reqs[j] = bids[num_total_blocks + j].read(
                            buffer[j], raw_block_size
                        );

                wait_all(reqs, current_num_blocks_per_batch);

                end = timestamp();
                elapsed = end - begin;
                total_size_read += current_batch_size;
                total_time_read += elapsed;
            }
            else
                elapsed = 0.0;

            if (!g_quiet)
                ss << std::setw(5) << std::setprecision(1)
                   << (double(current_batch_size) / MiB / elapsed)
                   << " MiB/s read";

            if (!g_quiet)
                LOG1 << ss.str();

#if CHECK_AFTER_READ
            for (size_t j = 0; j < current_num_blocks_per_batch; j++)
            {
                for (size_t i = 0; i < block_size; i++)
                {
                    if (buffer[j][i] != static_cast<uint32_t>(j * block_size + i))
                    {
                        size_t ibuf = i / current_batch_size_int;
                        size_t pos = i % current_batch_size_int;

                        LOG1 << "Error on disk " << ibuf << " position "
                             << std::hex << std::setw(8)
                             << offset + pos * sizeof(uint32_t)
                             << "  got: "
                             << std::hex << std::setw(8)
                             << buffer[j][i]
                             << " wanted: "
                             << std::hex << std::setw(8)
                             << (j * block_size + i)
                             << std::dec << std::endl;

                        i = (ibuf + 1) * current_batch_size_int; // jump to next
                    }
                }
            }
#endif
            if (timestamp() - ts_begin > g_time_limit)
                break;
        }
    }
    catch (const std::exception& ex)
    {
        LOG1 << ex.what();
    }

    LOG1 << "=============================================================================================\n"
         << "# Average over " << std::setw(7) << total_size_write / MiB << " MiB: "
         << std::setw(5) << std::setprecision(1)
         << (double(total_size_write) / MiB / total_time_write) << " MiB/s write, "
         << std::setw(5) << std::setprecision(1)
         << (double(total_size_read) / MiB / total_time_read) << " MiB/s read";

    std::cout << "RESULT"
              << (getenv("RESULT") ? getenv("RESULT") : "")
              << " size=" << size
              << " op=" << optrw
              << " block_size=" << raw_block_size
              << " batch_size=" << batch_size / raw_block_size
              << " offset=" << start_offset
              << " write_time=" << total_time_write
              << " read_time=" << total_time_read
              << " write_size=" << total_size_write
              << " read_size=" << total_size_read
              << " time=" << (total_time_write + total_time_read)
              << " total_size=" << (total_size_write + total_size_read)
              << std::endl;

    delete[] reqs;

    for (size_t j = 0; j < num_blocks_per_batch; ++j)
        foxxll::aligned_dealloc<4096>(buffer[j]);

    return 0;
}

int benchmark_disks(int argc, char* argv[])
{
    // parse command line

    tlx::CmdlineParser cp;

    external_size_type size = 0, offset = 0;
    unsigned int batch_size = 0;
    external_size_type block_size = 8 * MiB;
    std::string optrw = "rw", allocstr;

    cp.add_flag(
        'q', "quiet", g_quiet, "quiet processing"
    );

    cp.add_param_bytes(
        "size", size,
        "Amount of data to write/read from disks (e.g. 10GiB)"
    );
    cp.add_opt_param_string(
        "r|w", optrw,
        "Only read or write blocks (default: both write and read)"
    );
    cp.add_opt_param_string(
        "alloc", allocstr,
        "Block allocation strategy: random_cyclic, simple_random, fully_random, striping. (default: random_cyclic)"
    );

    cp.add_unsigned(
        'b', "batch", batch_size,
        "Number of blocks written/read in one batch (default: D * B)"
    );
    cp.add_bytes(
        'B', "block_size", block_size,
        "Size of blocks written in one syscall. (default: B = 8MiB)"
    );
    cp.add_bytes(
        'o', "offset", offset,
        "Starting offset of operation range. (default: 0)"
    );
    cp.add_double(
        'T', "time-limit", g_time_limit,
        "limit time of experiment (seconds)"
    );

    cp.set_description(
        "This program will benchmark the disks configured by the standard "
        ".foxxll disk configuration files mechanism. Blocks of 8 MiB are "
        "written and/or read in sequence using the block manager. The batch "
        "size describes how many blocks are written/read in one batch. The "
        "are taken from block_manager using given the specified allocation "
        "strategy. If size == 0, then writing/reading operation are done "
        "until an error occurs. "
    );

    if (!cp.process(argc, argv))
        return -1;

    if (allocstr.size())
    {
        if (allocstr == "random_cyclic")
            return benchmark_disks_alloc<foxxll::random_cyclic>(
                size, offset, batch_size, block_size, optrw
            );
        if (allocstr == "simple_random")
            return benchmark_disks_alloc<foxxll::simple_random>(
                size, offset, batch_size, block_size, optrw
            );
        if (allocstr == "fully_random")
            return benchmark_disks_alloc<foxxll::fully_random>(
                size, offset, batch_size, block_size, optrw
            );
        if (allocstr == "striping")
            return benchmark_disks_alloc<foxxll::striping>(
                size, offset, batch_size, block_size, optrw
            );

        LOG1 << "Unknown allocation strategy '" << allocstr << "'";
        cp.print_usage();
        return -1;
    }

    return benchmark_disks_alloc<foxxll::default_alloc_strategy>(
        size, offset, batch_size, block_size, optrw
    );
}

/**************************************************************************/
