############################################################################
#  tests/mng/CMakeLists.txt
#
#  Part of the STXXL. See http://stxxl.org
#
#  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

foxxll_build_test(test_async_schedule)
foxxll_build_test(test_aligned)
foxxll_build_test(test_block_alloc_strategy)
foxxll_build_test(test_block_manager)
foxxll_build_test(test_block_manager1)
foxxll_build_test(test_block_manager2)
foxxll_build_test(test_block_scheduler)
foxxll_build_test(test_bmlayer)
foxxll_build_test(test_buf_streams)
foxxll_build_test(test_config)
foxxll_build_test(test_pool_pair)
foxxll_build_test(test_prefetch_pool)
foxxll_build_test(test_read_write_pool)
foxxll_build_test(test_write_pool)

foxxll_test(test_async_schedule 3 100 1000 42)
foxxll_test(test_aligned)
foxxll_test(test_block_alloc_strategy)
foxxll_test(test_block_manager)
foxxll_test(test_block_manager1)
foxxll_test(test_block_manager2)
foxxll_test(test_block_scheduler)
foxxll_test(test_bmlayer)
foxxll_test(test_buf_streams)
foxxll_test(test_config)
foxxll_test(test_pool_pair)
foxxll_test(test_prefetch_pool)
foxxll_test(test_read_write_pool)
foxxll_test(test_write_pool)

add_define(test_block_manager "STXXL_VERBOSE_LEVEL=2")
add_define(test_block_manager1 "STXXL_VERBOSE_LEVEL=2")
add_define(test_block_manager2 "STXXL_VERBOSE_LEVEL=3")
add_define(test_prefetch_pool "STXXL_VERBOSE_LEVEL=2")
add_define(test_write_pool "STXXL_VERBOSE_LEVEL=1")
