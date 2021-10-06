############################################################################
#  CMakeLists.txt
#
#  Base CMake file for building stxxl with different options.
#
#  Part of FOXXLL. See http://foxxll.org
#
#  Copyright (C) 2013-2014 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

set(CMAKE_LEGACY_CYGWIN_WIN32 0) # needed to silence warning on cygwin

# require cmake 2.6.2 (but please use 2.8.x)
cmake_minimum_required(VERSION 2.6.2 FATAL_ERROR)

# the FOXXLL project
project(foxxll)

# for additional cmake scripts
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/misc/cmake)

# prohibit in-source builds
if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed, use a separate build directory.")
endif()

# default to Debug building for single-config generators
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message("Defaulting CMAKE_BUILD_TYPE to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")
endif()

# FOXXLL version string
set(FOXXLL_VERSION_MAJOR "1")
set(FOXXLL_VERSION_MINOR "4")
set(FOXXLL_VERSION_PATCH "99")
set(FOXXLL_VERSION_STRING "${FOXXLL_VERSION_MAJOR}.${FOXXLL_VERSION_MINOR}.${FOXXLL_VERSION_PATCH}")
set(FOXXLL_VERSION_PHASE "prerelease/${CMAKE_BUILD_TYPE}")

# read git directory (if it exists) and find git sha
if(EXISTS ${PROJECT_SOURCE_DIR}/.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe HEAD
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      OUTPUT_VARIABLE "FOXXLL_VERSION_GIT_REFSPEC"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      OUTPUT_VARIABLE "FOXXLL_VERSION_GIT_SHA1"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Detected git refspec ${FOXXLL_VERSION_GIT_REFSPEC} sha ${FOXXLL_VERSION_GIT_SHA1}")
  endif()
endif()

###############################################################################
# compilation options

# please document all build options in doc/install.doc:install_build_options

option(FOXXLL_BUILD_TESTS
  "Build all foxxll test programs" OFF)

option(FOXXLL_TRY_COMPILE_HEADERS
  "Test foxxll header files for self-sufficiency: try to compile them." OFF)

option(USE_VALGRIND
  "Run tests with valgrind, pre-initialize some memory buffers." OFF)

option(USE_MALLOC_COUNT
  "Compile all programs with included malloc_count heap profiler." OFF)

option(FOXXLL_USE_GCOV
  "Compile and run tests with gcov for coverage analysis." OFF)

### building shared and/or static libraries

# by default we currently only build a static library, since we do not aim to
# keep a stable binary interface.
option(FOXXLL_BUILD_STATIC_LIBS "Build static library version of libfoxxll" ON)
option(FOXXLL_BUILD_SHARED_LIBS "Build shared library version of libfoxxll" OFF)

### allow user to specify other installation paths

include(GNUInstallDirs)

set(FOXXLL_INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR}
  CACHE PATH "Installation directory for executables")
set(FOXXLL_INSTALL_LIB_DIR ${CMAKE_INSTALL_LIBDIR}
  CACHE PATH "Installation directory for libraries")
set(FOXXLL_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}
  CACHE PATH "Installation directory for header files")
set(FOXXLL_INSTALL_PKGCONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
  CACHE PATH "Installation directory for pkg-config file")

if(WIN32 AND NOT CYGWIN)
  set(FOXXLL_DEFAULT_INSTALL_CMAKE_DIR "CMake")
else()
  set(FOXXLL_DEFAULT_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/foxxll")
endif()
set(INSTALL_CMAKE_DIR "${FOXXLL_DEFAULT_INSTALL_CMAKE_DIR}"
  CACHE PATH "Installation directory for cmake files")

###############################################################################
# enable use of "make test"

enable_testing()

include(CTest)
set(CTEST_PROJECT_NAME "FOXXLL")

if(USE_VALGRIND)
  set(FOXXLL_WITH_VALGRIND 1)
  set(VALGRIND_OPTS --leak-check=full --error-exitcode=1 --suppressions=${PROJECT_SOURCE_DIR}/misc/valgrind.supp)
endif()

###############################################################################
# enable use of "make package"

if(NOT CPack_CMake_INCLUDED)
  # general package settings
  set(CPACK_PACKAGE_VENDOR "FOXXLL Maintainer Team")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library of external memory algorithms")
  set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE_1_0.txt")
  set(CPACK_PACKAGE_VERSION_MAJOR "${FOXXLL_VERSION_MAJOR}")
  set(CPACK_PACKAGE_VERSION_MINOR "${FOXXLL_VERSION_MINOR}")
  set(CPACK_PACKAGE_VERSION_PATCH "${FOXXLL_VERSION_PATCH}")

  # source packaging
  set(CPACK_SOURCE_GENERATOR "TGZ")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
  set(CPACK_SOURCE_IGNORE_FILES "/\\\\..*$;/build;${CPACK_SOURCE_IGNORE_FILES}")

  # binary packaging
  set(CPACK_GENERATOR "TGZ")

  include(CPack)
endif()

###############################################################################
# check platform

if(WIN32 OR WIN64 OR MINGW)
  set(FOXXLL_WINDOWS "1")
endif()

if(MSVC)
  set(FOXXLL_MSVC ${MSVC_VERSION})
endif()

###############################################################################
# enable warnings

if(MSVC)
  # Force to always compile with W3
  # This is the highest level recommended for production purposes
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W2" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2")
  endif()
  ### disable verbose warnings:
  # warning C4127: conditional expression is constant
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127")
  # warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4290")
  # warning C4250: '...' : inherits '...' via dominance
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4250")
  # warning C4512: assignment operator could not be generated (contains const members)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4512")
  # warning C4355: 'this' : used in base member initializer list
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4355")
  # disable lots of warnings about "unsecure" C runtime function
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  # disable lots of warnings about deprecated POSIX function names
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
  # disable lots of warnings about "unsecure" STL functions
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -pedantic -Wno-long-long")
  # detect -Wextra
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-Wextra CXX_HAS_FLAGS_WEXTRA)
  if(CXX_HAS_FLAGS_WEXTRA)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
  endif()
endif()

###############################################################################
# enable C++14 and more compiler features
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# more template depth for some tests and compilers
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-ftemplate-depth=1024 CXX_HAS_TEMPLATE_DEPTH)
if(CXX_HAS_TEMPLATE_DEPTH)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024")
endif()


if(APPLE)
  if (CMAKE_CXX_COMPILER MATCHES ".*clang[+][+]"
      OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    check_cxx_compiler_flag(-stdlib=libc++ CXX_HAS_STDLIB_LIBCXX)
    if(CXX_HAS_STDLIB_LIBCXX)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    else()
      message(SEND_ERROR "Compilation on MacOSX with clang requires libc++.")
    endif()
  endif()
endif(APPLE)

###############################################################################
# enable gcov coverage analysis with gcc

if(FOXXLL_USE_GCOV)
  # find programs
  find_program(GENHTML genhtml)
  find_program(LCOV lcov)

  if(NOT LCOV OR NOT GENHTML)
    message(SEND_ERROR "Coverage analysis requires lcov and genhtml programs.")
  endif()

  # add coverage anaylsis compile and link flags
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov")

  # add cached variable containing parameters for lcov/genhtml
  set(LCOV_FLAGS "" CACHE STRING "parameters for lcov")
  set(GENHTML_FLAGS --legend --no-branch-coverage
    CACHE STRING "parameters for genhtml")

  # custom target to run before tests
  add_custom_target(lcov-reset
    COMMAND ${LCOV} -q --directory ${CMAKE_BINARY_DIR} --zerocounters
    COMMENT "Resetting code coverage counters")

  # custom lcov target to run tests
  add_custom_target(lcov-runtests
    COMMAND ${CMAKE_CTEST_COMMAND} \${ARGS}
    DEPENDS ${ALL_TESTS} lcov-reset
    COMMENT "Running all unit tests")

  # get git version description
  execute_process(COMMAND git describe --tags
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE GITDESC
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # command sequence to gather, clean and generate HTML coverage report
  add_custom_target(lcov-html
    COMMAND ${LCOV} -q --directory . --capture --output-file lcov.info
    COMMAND ${LCOV} -q --remove lcov.info '/usr/*' ${LCOV_FLAGS} --output-file lcov-clean.info
    COMMAND ${GENHTML} -q -o coverage --title "FOXXLL ${GITDESC}" --prefix ${PROJECT_SOURCE_DIR} ${GENHTML_FLAGS} lcov-clean.info
    COMMENT "Capturing code coverage counters and create HTML coverage report"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # top-level target to run tests and generate coverage report
  add_custom_target(test-coverage
    COMMENT "Generate HTML coverage report "
    DEPENDS lcov-runtests lcov-html)

endif(FOXXLL_USE_GCOV)

###############################################################################
# find thread and random library

# Check for threading library and set linker flags accordingly
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_THREAD_LIBS_INIT}")

set(FOXXLL_DEPEND_LIBRARIES ${FOXXLL_DEPEND_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# for testing for c++ system include files
include(CheckIncludeFileCXX)

check_include_file_cxx(pthread.h HAVE_PTHREAD_H)

###############################################################################
# determine large file support
#     note: these flags need not be exported, all file operations must be
#     encapsulated within the library!

include(TestLargeFiles)
test_large_files(HAVE_LARGEFILES)

if(HAVE_LARGEFILES)
  add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES)
else()
  message(FATAL_ERROR "Large file support was not detectable.")
endif()

###############################################################################
# check for O_DIRECT flag

if(CYGWIN)
  #-tb O_DIRECT messes up cygwin
  set(FOXXLL_DIRECT_IO_OFF 1)
elseif(MSVC OR MINGW)
  # have FILE_FLAG_NO_BUFFERING on Windows
  set(FOXXLL_DIRECT_IO_OFF 0)
elseif(APPLE)
  # we do not have to test anything, all is well.
  set(FOXXLL_DIRECT_IO_OFF 0)
else()

  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("
#include <unistd.h>
#include <fcntl.h>
int main() { return ((int)O_DIRECT) != 0; }
" FOXXLL_HAVE_O_DIRECT)

  if(FOXXLL_HAVE_O_DIRECT)
    set(FOXXLL_DIRECT_IO_OFF 0)
  else()
    set(FOXXLL_DIRECT_IO_OFF 1)
  endif()

endif()

if(FOXXLL_DIRECT_IO_OFF)
  message(WARNING
    "The open() flag O_DIRECT was disabled!\n"
    "This means that FOXXLL cannot bypass the system cache on your system, which may degrade performance.")
endif()

###############################################################################
# check for mmap() function in sys/mman.h

include(CheckSymbolExists)
check_symbol_exists(mmap "sys/mman.h" FOXXLL_HAVE_MMAP_FILE)

###############################################################################
# check for Linux aio syscalls

include(CheckCXXSourceCompiles)
check_cxx_source_compiles(
  "#include <unistd.h>
   #include <sys/syscall.h>
   #include <linux/aio_abi.h>
   int main() {
       aio_context_t context;
       long r = syscall(SYS_io_setup, 5, &context);
       return (r == 0) ? 0 : -1;
   }"
   FOXXLL_HAVE_LINUXAIO_FILE)

###############################################################################
# test for additional includes and features used by some foxxll_tool components

if(USE_MALLOC_COUNT)
  # malloc_count requires the linux dl (dynamic linker) library
  find_library(DL_LIBRARIES NAMES dl)
  set(FOXXLL_DEPEND_LIBRARIES ${FOXXLL_DEPEND_LIBRARIES} ${DL_LIBRARIES})
endif()

###############################################################################
# find tlx

if(NOT TARGET tlx)
  add_subdirectory(extlib/tlx)
endif()

set(FOXXLL_DEPEND_INCLUDE_DIRS
  ${TLX_INCLUDE_DIRS} ${FOXXLL_DEPEND_INCLUDE_DIRS})
set(FOXXLL_DEPEND_LIBRARIES
  ${TLX_LIBRARIES} ${FOXXLL_DEPEND_LIBRARIES})

###############################################################################
# configure environment for building

# create config.h with define switches _inside binary dir_!
configure_file(
  ${PROJECT_SOURCE_DIR}/foxxll/config.hpp.in
  ${PROJECT_BINARY_DIR}/foxxll/config.hpp)

# Custom build type "ReleaseWithAssert".
string(REGEX REPLACE "[/-][dD][^/-]*NDEBUG" "" CMAKE_C_FLAGS_RELWITHASSERT_INIT_T "${CMAKE_C_FLAGS_RELWITHDEBINFO_INIT}")
string(REGEX REPLACE "([/-])O[23]" "\\1O1" CMAKE_C_FLAGS_RELWITHASSERT_INIT "${CMAKE_C_FLAGS_RELWITHASSERT_INIT_T}")
string(REGEX REPLACE "[/-][dD][^/-]*NDEBUG" "" CMAKE_CXX_FLAGS_RELWITHASSERT_INIT_T "${CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT}")
string(REGEX REPLACE "([/-])O[23]" "\\1O1" CMAKE_CXX_FLAGS_RELWITHASSERT_INIT "${CMAKE_CXX_FLAGS_RELWITHASSERT_INIT_T}")
set(CMAKE_C_FLAGS_RELWITHASSERT "${CMAKE_C_FLAGS_RELWITHASSERT_INIT}"
  CACHE STRING "C flags for release with assert builds.")
set(CMAKE_CXX_FLAGS_RELWITHASSERT "${CMAKE_CXX_FLAGS_RELWITHASSERT_INIT}"
  CACHE STRING "C++ flags for release with assert builds.")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHASSERT ""
  CACHE STRING "Linker flags for release with assert builds.")
mark_as_advanced( CMAKE_CXX_FLAGS_RELWITHASSERT CMAKE_C_FLAGS_RELWITHASSERT CMAKE_EXE_LINKER_FLAGS_RELWITHASSERT)
# add possible values of CMAKE_BUILD_TYPE type: RelWithAssert
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "None" "Debug" "Release" "RelWithDebInfo" "MinSizeRel" "RelWithAssert")

# globally adds top-level include directories
set(FOXXLL_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/ ${PROJECT_BINARY_DIR}/
  ${FOXXLL_DEPEND_INCLUDE_DIRS})

# for targets using foxxll library
set(FOXXLL_LIBRARIES foxxll ${FOXXLL_DEPEND_LIBRARIES})

# export FOXXLL_INCLUDE_DIRS and FOXXLL_LIBRARIES to global CACHE
set(FOXXLL_CXX_FLAGS "${FOXXLL_CXX_FLAGS}" CACHE STRING "Compiler flags for FOXXLL")
set(FOXXLL_INCLUDE_DIRS "${FOXXLL_INCLUDE_DIRS}" CACHE STRING "Include paths for FOXXLL")
set(FOXXLL_LIBRARIES "${FOXXLL_LIBRARIES}" CACHE STRING "Libraries to link for FOXXLL")

# build libfoxxll
add_subdirectory(foxxll)

###############################################################################
# macros for building foxxll programs and tests

# macro for building foxxll programs
macro(foxxll_build_tool PROGNAME)

  add_executable(${PROGNAME} ${PROGNAME}.cpp ${ARGN})
  target_link_libraries(${PROGNAME} ${FOXXLL_LIBRARIES})

endmacro(foxxll_build_tool)

# macro for building foxxll tests
macro(foxxll_build_test TESTNAME)

  if(FOXXLL_BUILD_TESTS)
    add_executable(foxxll_${TESTNAME} ${TESTNAME}.cpp ${ARGN})
    target_link_libraries(foxxll_${TESTNAME} ${FOXXLL_LIBRARIES})
  endif(FOXXLL_BUILD_TESTS)

endmacro(foxxll_build_test)

# macro for registering foxxll tests
macro(foxxll_test TESTNAME)

  if(FOXXLL_BUILD_TESTS)
    set(TESTFULLNAME foxxll_${TESTNAME} ${ARGN})
    string(REPLACE ";" "_" TESTFULLNAME "${TESTFULLNAME}") # stringify list

    if(USE_VALGRIND)
      # prepend valgrind call
      add_test(${TESTFULLNAME} /usr/bin/valgrind ${VALGRIND_OPTS} ./${TESTNAME} ${ARGN})
      set_tests_properties(${TESTFULLNAME} PROPERTIES TIMEOUT 7200)
    else()
      add_test(${TESTFULLNAME} foxxll_${TESTNAME} ${ARGN})
      set_tests_properties(${TESTFULLNAME} PROPERTIES TIMEOUT 3600)
    endif()

  endif(FOXXLL_BUILD_TESTS)

endmacro(foxxll_test)

# macro for setting additional defines for targets
macro(add_define PROGNAME)

  if(TARGET ${PROGNAME})
    set_property(TARGET ${PROGNAME} APPEND PROPERTY COMPILE_DEFINITIONS ${ARGN})
  else()
    if(FOXXLL_BUILD_TESTS)
      message("Ignoring add_define(${PROGNAME} ${ARGN}) for unknown target ${PROGNAME}")
    endif(FOXXLL_BUILD_TESTS)
  endif()

endmacro(add_define TESTNAME)

###############################################################################
# figure out FOXXLL_TEST_DISKDIR for tests

if(FOXXLL_BUILD_TESTS)

  if(DEFINED ENV{FOXXLL_TEST_DISKDIR})
    set(FOXXLL_TEST_DISKDIR "$ENV{FOXXLL_TEST_DISKDIR}")
  elseif(NOT DEFINED FOXXLL_TEST_DISKDIR)
    set(FOXXLL_TEST_DISKDIR ".")
  endif()

  message(STATUS "Using FOXXLL_TEST_DISKDIR ${FOXXLL_TEST_DISKDIR} for tests")

endif(FOXXLL_BUILD_TESTS)

###############################################################################
# install header files

install(DIRECTORY foxxll
  DESTINATION ${FOXXLL_INSTALL_INCLUDE_DIR}
  FILES_MATCHING PATTERN "*.hpp")

# also copy the config file with build options!
install(FILES ${PROJECT_BINARY_DIR}/foxxll/config.hpp
  DESTINATION ${INSTALL_INCLUDE_DIR}/foxxll/)

###############################################################################
# prepare pkg-config file

configure_file(misc/cmake/foxxll.pc
  "${PROJECT_BINARY_DIR}/${FOXXLL_LIBNAME}.pc" @ONLY)

# copy the foxxll.pc file into lib/pkgconfig
if(FOXXLL_INSTALL_PKGCONFIG_DIR)
  install(FILES ${PROJECT_BINARY_DIR}/${FOXXLL_LIBNAME}.pc
    DESTINATION ${FOXXLL_INSTALL_PKGCONFIG_DIR})
endif()

###############################################################################
# export targets to cmake project config file

if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 2.6)
  # register package for use from the global CMake-registry
  export(PACKAGE foxxll)
endif()

# add foxxll library targets to the build tree export set
export(TARGETS ${FOXXLL_EXPORTED_LIBS}
  FILE "${PROJECT_BINARY_DIR}/foxxll-targets.cmake")

# calculate absolute install paths
foreach(dir LIB BIN INCLUDE CMAKE)
  set(ABS_INSTALL_${dir}_DIR "${INSTALL_${dir}_DIR}")
  if(NOT IS_ABSOLUTE "${ABS_INSTALL_${dir}_DIR}")
    set(ABS_INSTALL_${dir}_DIR "${CMAKE_INSTALL_PREFIX}/${ABS_INSTALL_${dir}_DIR}")
  endif()
endforeach()

# calculate final installed include path relative to cmake install path
file(RELATIVE_PATH REL_INCLUDE_DIR "${ABS_INSTALL_CMAKE_DIR}" "${ABS_INSTALL_INCLUDE_DIR}")

# create common foxxll-config.cmake file
configure_file(misc/cmake/foxxll-version.cmake.in
  "${PROJECT_BINARY_DIR}/foxxll-version.cmake" @ONLY)

# create foxxll-version.cmake file for the build tree
set(CONF_INCLUDE_DIRS "${FOXXLL_INCLUDE_DIRS}")
configure_file(misc/cmake/foxxll-config.cmake.in
  "${PROJECT_BINARY_DIR}/foxxll-config.cmake" @ONLY)

# create foxxll-version.cmake file for the install tree
set(CONF_INCLUDE_DIRS "\${FOXXLL_CMAKE_DIR}/${REL_INCLUDE_DIR}")
configure_file(misc/cmake/foxxll-config.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/foxxll-config.cmake" @ONLY)

# install the foxxll-config.cmake and foxxll-version.cmake
install(FILES
  "${PROJECT_BINARY_DIR}/foxxll-version.cmake"
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/foxxll-config.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}")

# Install the export set for use with the install-tree
install(EXPORT foxxll-targets DESTINATION "${INSTALL_CMAKE_DIR}")

###############################################################################
# build tests and tools

add_subdirectory(tools)
add_subdirectory(tests)

###############################################################################
# cmake script FOXXLL_TRY_COMPILE all foxxll header files

if(FOXXLL_TRY_COMPILE_HEADERS)

  file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/try_compile")

  set(header_files "")
  file(GLOB_RECURSE header_files
    RELATIVE "${PROJECT_SOURCE_DIR}"  "${PROJECT_SOURCE_DIR}/foxxll/*.hpp")

  foreach(header_file ${header_files})
    # replace / and . with _ to get a valid target and file name
    string(REPLACE "/" "_" compile_name "${header_file}")
    string(REPLACE "." "_" compile_name "${compile_name}")

    set(target_name "foxxll_try_compile_${compile_name}")
    set(source_name "${PROJECT_BINARY_DIR}/try_compile/${target_name}.cpp")

    file(WRITE ${source_name}
      "#include <${header_file}>
       int main() { return 0; }")

    add_executable(${target_name} ${source_name})
    target_link_libraries(${target_name} ${FOXXLL_LIBRARIES})
    set_property(TARGET ${target_name}
      PROPERTY RUNTIME_OUTPUT_DIRECTORY "try_compile/")

  endforeach()

endif(FOXXLL_TRY_COMPILE_HEADERS)

################################################################################
