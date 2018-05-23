/***************************************************************************
 *  tests/mng/test_pool_pair.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example mng/test_pool_pair.cpp

#include <iostream>

#include <tlx/logger.hpp>

#include <foxxll/common/die_with_message.hpp>
#include <foxxll/mng.hpp>
#include <foxxll/mng/prefetch_pool.hpp>
#include <foxxll/mng/write_pool.hpp>

#define BLOCK_SIZE (1024 * 512)

struct MyType
{
    int integer;
    char chars[5];
};

using block_type = foxxll::typed_block<BLOCK_SIZE, MyType>;

// forced instantiation
template class foxxll::typed_block<BLOCK_SIZE, MyType>;
template class foxxll::prefetch_pool<block_type>;
template class foxxll::write_pool<block_type>;

int main()
{
    foxxll::block_manager* bm = foxxll::block_manager::get_instance();
    foxxll::default_alloc_strategy alloc;

    {
        LOG1 << "Write-After-Write coherence test";
        foxxll::prefetch_pool<block_type> p_pool(2);
        foxxll::write_pool<block_type> w_pool(10);
        block_type* blk;
        block_type::bid_type bid;

        bm->new_block(alloc, bid);

        // write the block for the first time
        blk = w_pool.steal();
        (*blk)[0].integer = 42;
        w_pool.write(blk, bid);

        // read the block
        blk = w_pool.steal();
        p_pool.read(blk, bid)->wait();
        delete blk;

        // write the block for the second time
        blk = w_pool.steal();
        (*blk)[0].integer = 23;
        w_pool.write(blk, bid);

        // hint the block
        p_pool.hint(bid, w_pool); // flush w_pool

        // get the hinted block
        blk = w_pool.steal();
        p_pool.read(blk, bid)->wait();

        die_with_message_unless((*blk)[0].integer == 23,
                                "WRITE-AFTER-WRITE COHERENCE FAILURE");

        w_pool.add(blk);
        bm->delete_block(bid);
    }

    {
        LOG1 << "Write-After-Hint coherence test #1";
        foxxll::prefetch_pool<block_type> p_pool(1);
        foxxll::write_pool<block_type> w_pool(1);
        block_type* blk;
        block_type::bid_type bid;

        bm->new_block(alloc, bid);
        blk = w_pool.steal();
        (*blk)[0].integer = 42;
        w_pool.write(blk, bid);
        blk = w_pool.steal(); // flush w_pool

        // hint the block
        p_pool.hint(bid);

        // update the hinted block
        (*blk)[0].integer = 23;
        w_pool.write(blk, bid);
        p_pool.invalidate(bid);
        blk = w_pool.steal(); // flush w_pool

        // get the hinted block
        p_pool.read(blk, bid)->wait();

        die_with_message_unless((*blk)[0].integer == 23,
                                "WRITE-AFTER-HINT COHERENCE FAILURE");

        w_pool.add(blk);
        bm->delete_block(bid);
    }

    {
        LOG1 << "Write-After-Hint coherence test #2";
        foxxll::prefetch_pool<block_type> p_pool(1);
        foxxll::write_pool<block_type> w_pool(1);
        block_type* blk;
        block_type::bid_type bid;

        bm->new_block(alloc, bid);
        blk = w_pool.steal();
        (*blk)[0].integer = 42;
        w_pool.write(blk, bid);

        // hint the block
        p_pool.hint(bid, w_pool); // flush w_pool

        // update the hinted block
        blk = w_pool.steal();
        (*blk)[0].integer = 23;
        w_pool.write(blk, bid);
        p_pool.invalidate(bid);
        blk = w_pool.steal(); // flush w_pool

        // get the hinted block
        p_pool.read(blk, bid)->wait();

        die_with_message_unless((*blk)[0].integer == 23,
                                "WRITE-AFTER-HINT COHERENCE FAILURE");

        w_pool.add(blk);
        bm->delete_block(bid);
    }

    return 0;
}

/**************************************************************************/
