############################################################################
#  tools/CMakeLists.txt
#
#  Part of FOXXLL. See http://foxxll.org
#
#  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

foxxll_build_tool(foxxll_tool
  create_files.cpp
  benchmark_disks.cpp
  benchmark_files.cpp
  benchmark_disks_random.cpp
  )

install(TARGETS foxxll_tool RUNTIME DESTINATION ${INSTALL_BIN_DIR})

############################################################################
