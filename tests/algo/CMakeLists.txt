############################################################################
#  tests/algo/CMakeLists.txt
#
#  Part of the STXXL. See http://stxxl.org
#
#  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

foxxll_build_test(test_asch)

foxxll_test(test_asch 3 100 1000 42)

############################################################################
