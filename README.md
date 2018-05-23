# FOXXLL
Travis-CI Status of branch `master`: [![Travis-CI Status](https://travis-ci.org/stxxl/foxxll.svg?branch=master)](https://travis-ci.org/stxxl/foxxll)

Coverity Result of branch `coverity_scan`: [![Coverity Status](https://scan.coverity.com/projects/15041/badge.svg?flat=1)](https://scan.coverity.com/projects/foxxll)


FOXXLL is the **Fo**undation of ST**XX**L and Thri**LL** and offers solutions to (mostly) external-memory related issues.
The lowest layer, the Asynchronous I/O primitives layer (AIO layer), abstracts away the details of how asynchronous I/O is performed on a particular operating system.
It contains a number of drivers to access files (and raw devices) under Linux (syscall, linuxaio, memory-mapped access), OSX and Windows.
It also features an EM emulation layer for fast development and collects extensive statistics on the IO performance of the system.

The Block Management layer (BM layer) provides a programming interface emulating the parallel disk model.
The BM layer provides an abstraction for a fundamental concept in the external memory algorithm design — a block of elements.
The block manager implements block allocation/deallocation, allowing several block-to-disk assignment strategies: striping, randomized striping, randomized cycling, etc.
The layer provides an implementation of parallel disk buffered writing, optimal prefetching, and block caching.
The implementations are fully asynchronous and designed to explicitly support overlapping between I/O and computation.

## LICENSE TERMS

FOXXLL is distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
