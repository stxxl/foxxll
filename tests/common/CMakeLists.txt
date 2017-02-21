############################################################################
#  tests/common/CMakeLists.txt
#
#  Part of the STXXL. See http://stxxl.org
#
#  Copyright (C) 2013-2014 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

foxxll_build_test(test_cmdline)
foxxll_build_test(test_log2)
foxxll_build_test(test_uint_types)

foxxll_test(test_cmdline)
foxxll_test(test_log2)
foxxll_test(test_uint_types)

############################################################################
