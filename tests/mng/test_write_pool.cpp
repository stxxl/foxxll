/***************************************************************************
 *  tests/mng/test_write_pool.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example mng/test_write_pool.cpp

#include <iostream>

#include <foxxll/mng.hpp>
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
template class foxxll::write_pool<block_type>;

int main()
{
    foxxll::write_pool<block_type> pool(100);
    pool.resize(10);
    pool.resize(5);
    block_type* blk = new block_type;
    block_type::bid_type bid;
    foxxll::block_manager::get_instance()->new_block(foxxll::single_disk(), bid);
    pool.write(blk, bid)->wait();
    delete blk;
}
