/***************************************************************************
 *  tests/mng/test_block_manager.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2018 Manuel Penschuck <foxxll@manuel.jetzt>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example mng/test_mng.cpp
//! This is an example of use of completion handlers, \c foxxll::block_manager, and
//! \c foxxll::typed_block

#include <iostream>
#include <memory>
#include <vector>

#include <foxxll/io.hpp>
#include <foxxll/mng.hpp>
#include <foxxll/verbose.hpp>

constexpr size_t BLOCK_SIZE = 512 * 1024;

struct MyType
{
    size_t integer;
    //char chars[4];
    ~MyType() { }
};

struct my_handler
{
    void operator () (foxxll::request* req, bool /* success */)
    {
        STXXL_MSG(req << " done, type=" << req->io_type());
    }
};

template class foxxll::typed_block<BLOCK_SIZE, int>;    // forced instantiation
template class foxxll::typed_block<BLOCK_SIZE, MyType>; // forced instantiation

using block_type = foxxll::typed_block<BLOCK_SIZE, MyType>;

int main()
{
    STXXL_MSG(sizeof(MyType) << " " << (BLOCK_SIZE % sizeof(MyType)));
    STXXL_MSG(sizeof(block_type) << " " << BLOCK_SIZE);

    constexpr size_t nblocks = 2;
    foxxll::BIDArray<BLOCK_SIZE> bids(nblocks);

    std::unique_ptr<foxxll::request_ptr[]> reqs(new foxxll::request_ptr[nblocks]);
    foxxll::block_manager* bm = foxxll::block_manager::get_instance();
    bm->new_blocks(foxxll::striping(), bids.begin(), bids.end());

    std::unique_ptr<block_type[]> block(new block_type[nblocks]);

    STXXL_MSG(std::hex);
    STXXL_MSG("Allocated block address    : " << (size_t)(block.get()));
    STXXL_MSG("Allocated block address + 1: " << (size_t)(block.get() + 1));
    STXXL_MSG(std::dec);

    for (size_t i = 0; i < nblocks; i++) {
        for (size_t j = 0; j < block_type::size; ++j) {
            block[i].elem[j].integer = i + j;
        }
    }

    for (size_t i = 0; i < nblocks; ++i)
        reqs[i] = block[i].write(bids[i], my_handler());

    std::cout << "Waiting " << std::endl;
    wait_all(reqs.get(), nblocks);

    for (size_t i = 0; i < nblocks; i++) {
        for (size_t j = 0; j < block_type::size; ++j) {
            block[i].elem[j].integer = 0xdeadbeaf;
        }
    }

    for (size_t i = 0; i < nblocks; ++i)
    {
        reqs[i] = block[i].read(bids[i], my_handler());
        reqs[i]->wait();
        for (size_t j = 0; j < block_type::size; ++j)
        {
            STXXL_CHECK2(i + j == block[i].elem[j].integer,
                         "Error in block " << std::hex << i << " pos: " << j
                                           << " value read: " << block[i].elem[j].integer);
        }
    }

    bm->delete_blocks(bids.begin(), bids.end());
}
