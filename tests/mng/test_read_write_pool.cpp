/***************************************************************************
 *  tests/mng/test_read_write_pool.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example mng/test_read_write_pool.cpp

#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/common/die_with_message.hpp>
#include <foxxll/mng.hpp>
#include <foxxll/mng/read_write_pool.hpp>

#define BLOCK_SIZE (1024 * 512)

struct MyType
{
    int integer;
    char chars[5];
};

using block_type = foxxll::typed_block<BLOCK_SIZE, MyType>;

// forced instantiation
template class foxxll::typed_block<BLOCK_SIZE, MyType>;
template class foxxll::read_write_pool<block_type>;

int main()
{
    foxxll::block_manager* bm = foxxll::block_manager::get_instance();
    foxxll::default_alloc_strategy alloc;

    {
        LOG1 << "Write-After-Write coherence test";
        foxxll::read_write_pool<block_type> pool(2, 10);
        block_type* blk;
        block_type::bid_type bid;

        bm->new_block(alloc, bid);

        // write the block for the first time
        blk = pool.steal();
        (*blk)[0].integer = 42;
        pool.write(blk, bid);

        // read the block
        blk = pool.steal();
        pool.read(blk, bid)->wait();
        delete blk;

        // write the block for the second time
        blk = pool.steal();
        (*blk)[0].integer = 23;
        pool.write(blk, bid);

        // hint the block
        pool.hint(bid); // flush w_pool

        // get the hinted block
        blk = pool.steal();
        pool.read(blk, bid)->wait();

        die_with_message_unless((*blk)[0].integer == 23,
                      "WRITE-AFTER-WRITE COHERENCE FAILURE");

        pool.add(blk);
        bm->delete_block(bid);
    }

    {
        LOG1 << "Write-After-Hint coherence test #1";
        foxxll::read_write_pool<block_type> pool(1, 1);
        block_type* blk;
        block_type::bid_type bid;

        bm->new_block(alloc, bid);
        blk = pool.steal();
        (*blk)[0].integer = 42;
        pool.write(blk, bid);
        blk = pool.steal(); // flush w_pool

        // hint the block
        pool.hint(bid);

        // update the hinted block
        (*blk)[0].integer = 23;
        pool.write(blk, bid);
        blk = pool.steal(); // flush w_pool

        // get the hinted block
        pool.read(blk, bid)->wait();

        die_with_message_unless((*blk)[0].integer == 23,
                      "WRITE-AFTER-HINT COHERENCE FAILURE");

        pool.add(blk);
        bm->delete_block(bid);
    }

    {
        LOG1 << "Write-After-Hint coherence test #2";
        foxxll::read_write_pool<block_type> pool(1, 1);
        block_type* blk;
        block_type::bid_type bid;

        bm->new_block(alloc, bid);
        blk = pool.steal();
        (*blk)[0].integer = 42;
        pool.write(blk, bid);

        // hint the block
        pool.hint(bid);

        // update the hinted block
        blk = pool.steal();
        (*blk)[0].integer = 23;
        pool.write(blk, bid);
        blk = pool.steal(); // flush w_pool

        // get the hinted block
        pool.read(blk, bid)->wait();

        die_with_message_unless((*blk)[0].integer == 23,
                      "WRITE-AFTER-HINT COHERENCE FAILURE");

        pool.add(blk);
        bm->delete_block(bid);
    }

    return 0;
}
// vim: et:ts=4:sw=4
