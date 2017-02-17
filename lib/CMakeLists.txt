############################################################################
#  lib/CMakeLists.txt
#
#  CMake file for libstxxl.a/so/lib
#
#  Part of the STXXL. See http://stxxl.org
#
#  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

set(LIBFOXXLL_SOURCES

  common/cmdline.cpp
  common/exithandler.cpp
  common/log.cpp
  common/utils.cpp
  common/verbose.cpp
  common/version.cpp

  io/create_file.cpp
  io/disk_queued_file.cpp
  io/file.cpp
  io/fileperblock_file.cpp
  io/iostats.cpp
  io/memory_file.cpp
  io/request.cpp
  io/request_queue_impl_1q.cpp
  io/request_queue_impl_qwqr.cpp
  io/request_queue_impl_worker.cpp
  io/request_with_state.cpp
  io/request_with_waiters.cpp
  io/serving_request.cpp
  io/syscall_file.cpp
  io/ufs_file_base.cpp
  io/wfs_file_base.cpp
  io/wincall_file.cpp

  mng/async_schedule.cpp
  mng/block_manager.cpp
  mng/config.cpp
  mng/disk_block_allocator.cpp

  )

if(NOT MSVC)
  # additional sources for non Visual Studio builds
  set(LIBFOXXLL_SOURCES ${LIBFOXXLL_SOURCES}
    io/mmap_file.cpp
    )
endif(NOT MSVC)

if(STXXL_HAVE_LINUXAIO_FILE)
  # additional sources fo LinuxAIO fileio access method
  set(LIBFOXXLL_SOURCES ${LIBFOXXLL_SOURCES}
    io/linuxaio_file.cpp
    io/linuxaio_queue.cpp
    io/linuxaio_request.cpp
    )
endif()

if(USE_MALLOC_COUNT)
  # enable light-weight heap profiling tool malloc_count
  set(LIBFOXXLL_SOURCES ${LIBFOXXLL_SOURCES}
    utils/malloc_count.cpp
    )

  set_source_files_properties(utils/malloc_count.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-pedantic")

endif(USE_MALLOC_COUNT)

# tell top-level cmakelists which library we build
set(FOXXLL_EXPORTED_LIBS foxxll)

# we name debug library builds "foxxll_debug" and release builds "foxxll"
string(TOLOWER "foxxll_${CMAKE_BUILD_TYPE}" FOXXLL_LIBNAME)
if(FOXXLL_LIBNAME STREQUAL "foxxll_release" OR FOXXLL_LIBNAME STREQUAL "foxxll_")
  set(FOXXLL_LIBNAME "foxxll")
endif()

if(BUILD_SHARED_LIBS)

  # build foxxll SHARED library and link all programs again it
  add_library(foxxll SHARED ${LIBFOXXLL_SOURCES})
  set_target_properties(foxxll PROPERTIES OUTPUT_NAME "${FOXXLL_LIBNAME}")
  set_target_properties(foxxll PROPERTIES VERSION "${FOXXLL_VERSION_STRING}")

  target_compile_definitions(foxxll PUBLIC ${FOXXLL_DEFINITIONS})
  target_include_directories(foxxll PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include/>)
  #target_include_directories(foxxll SYSTEM PUBLIC ${FOXXLL_INCLUDE_DIRS})
  target_link_libraries(foxxll ${FOXXLL_EXTRA_LIBRARIES})

  install(TARGETS foxxll
    EXPORT foxxll-targets
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR})

  if(BUILD_STATIC_LIBS)
    # but also build STATIC library
    add_library(foxxll_static STATIC ${LIBFOXXLL_SOURCES})
    set_target_properties(foxxll_static PROPERTIES OUTPUT_NAME "${FOXXLL_LIBNAME}")
    set_target_properties(foxxll_static PROPERTIES VERSION "${FOXXLL_VERSION_STRING}")

    target_compile_definitions(foxxll_static PUBLIC ${FOXXLL_DEFINITIONS})
    target_include_directories(foxxll_static PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
      $<INSTALL_INTERFACE:include/>)
    #target_include_directories(foxxll_static SYSTEM PUBLIC ${FOXXLL_INCLUDE_DIRS})
    target_link_libraries(foxxll_static ${FOXXLL_EXTRA_LIBRARIES})

    install(TARGETS foxxll_static
      EXPORT foxxll-targets
      ARCHIVE DESTINATION ${INSTALL_LIB_DIR})

    # we build both shared and static, export both
    set(FOXXLL_EXPORTED_LIBS foxxll foxxll_static)
  endif()

else()

  if(NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "You must build either a static or shared FOXXLL library")
  endif()

  # build foxxll STATIC library and link all programs again it
  add_library(foxxll STATIC ${LIBFOXXLL_SOURCES})
  set_target_properties(foxxll PROPERTIES OUTPUT_NAME "${FOXXLL_LIBNAME}")
  set_target_properties(foxxll PROPERTIES VERSION "${FOXXLL_VERSION_STRING}")

  target_compile_definitions(foxxll PUBLIC ${FOXXLL_DEFINITIONS})
  target_include_directories(foxxll PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include/>)
  #target_include_directories(foxxll SYSTEM PUBLIC ${FOXXLL_INCLUDE_DIRS})
  target_link_libraries(foxxll ${FOXXLL_EXTRA_LIBRARIES})

  install(TARGETS foxxll
    EXPORT foxxll-targets
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR})

endif()

# export variables to top-level cmakelists
set(FOXXLL_LIBNAME ${FOXXLL_LIBNAME} PARENT_SCOPE)
set(FOXXLL_EXPORTED_LIBS ${FOXXLL_EXPORTED_LIBS} PARENT_SCOPE)
