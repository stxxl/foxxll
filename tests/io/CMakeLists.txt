############################################################################
#  tests/io/CMakeLists.txt
#
#  Part of FOXXLL. See http://foxxll.org
#
#  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

foxxll_build_test(test_cancel)
foxxll_build_test(test_io)
foxxll_build_test(test_io_sizes)

foxxll_test(test_io "${STXXL_TMPDIR}")

foxxll_test(test_cancel syscall
  "${STXXL_TMPDIR}/testdisk_cancel_syscall")
# TODO: clean up after fileperblock_syscall
foxxll_test(test_cancel fileperblock_syscall
  "${STXXL_TMPDIR}/testdisk_cancel_fpb_syscall")

if(FOXXLL_HAVE_MMAP_FILE)
  foxxll_test(test_cancel mmap
    "${STXXL_TMPDIR}/testdisk_cancel_mmap")
  foxxll_test(test_cancel fileperblock_mmap
    "${STXXL_TMPDIR}/testdisk_cancel_fpb_mmap")
endif(FOXXLL_HAVE_MMAP_FILE)

if(FOXXLL_HAVE_LINUXAIO_FILE)
  foxxll_test(test_cancel linuxaio
    "${STXXL_TMPDIR}/testdisk_cancel_linuxaio")
endif(FOXXLL_HAVE_LINUXAIO_FILE)

foxxll_test(test_cancel memory
  "${STXXL_TMPDIR}/testdisk_cancel_memory")

foxxll_test(test_io_sizes syscall
  "${STXXL_TMPDIR}/testdisk_io_sizes_syscall" 1073741824)
if(FOXXLL_HAVE_MMAP_FILE)
  foxxll_test(test_io_sizes mmap
    "${STXXL_TMPDIR}/testdisk_io_sizes_mmap" 1073741824)
endif(FOXXLL_HAVE_MMAP_FILE)
if(FOXXLL_HAVE_LINUXAIO_FILE)
  foxxll_test(test_io_sizes linuxaio
    "${STXXL_TMPDIR}/testdisk_io_sizes_linxaio" 1073741824)
endif(FOXXLL_HAVE_LINUXAIO_FILE)

if(FOXXLL_HAVE_MMAP_FILE)
  foxxll_build_test(test_mmap)
  foxxll_test(test_mmap)
endif(FOXXLL_HAVE_MMAP_FILE)
