/***************************************************************************
 *  tests/mng/test_block_manager1.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <foxxll/io.hpp>
#include <foxxll/mng.hpp>

int main()
{
    using block_type = foxxll::typed_block<128* 1024, double>;
    std::vector<block_type::bid_type> bids(32);
    std::vector<foxxll::request_ptr> requests;
    foxxll::block_manager* bm = foxxll::block_manager::get_instance();
    bm->new_blocks(foxxll::striping(), bids.begin(), bids.end());
    std::vector<block_type, foxxll::new_alloc<block_type> > blocks(32);
    for (int vIndex = 0; vIndex < 32; ++vIndex) {
        for (size_t vIndex2 = 0; vIndex2 < block_type::size; ++vIndex2) {
            blocks[vIndex][vIndex2] = static_cast<double>(vIndex2);
        }
    }
    for (int vIndex = 0; vIndex < 32; ++vIndex) {
        requests.push_back(blocks[vIndex].write(bids[vIndex]));
    }
    foxxll::wait_all(requests.begin(), requests.end());
    bm->delete_blocks(bids.begin(), bids.end());
    return 0;
}
