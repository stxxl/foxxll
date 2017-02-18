/***************************************************************************
 *  tests/mng/test_config.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <foxxll/mng.hpp>
#include <foxxll/verbose.hpp>

void test1()
{
    // test disk_config parser:

    foxxll::disk_config cfg;

    cfg.parse_line("disk=/var/tmp/foxxll.tmp, 100 GiB , syscall unlink direct=on");

    STXXL_CHECK_EQUAL(cfg.path, "/var/tmp/foxxll.tmp");
    STXXL_CHECK_EQUAL(cfg.size, 100 * 1024 * 1024 * uint64_t(1024));
    STXXL_CHECK_EQUAL(cfg.fileio_string(), "syscall direct=on unlink_on_open");

    // test disk_config parser:

    cfg.parse_line("disk=/var/tmp/foxxll.tmp, 100 , wincall queue=5 delete_on_exit direct=on");

    STXXL_CHECK_EQUAL(cfg.path, "/var/tmp/foxxll.tmp");
    STXXL_CHECK_EQUAL(cfg.size, 100 * 1024 * uint64_t(1024));
    STXXL_CHECK_EQUAL(cfg.fileio_string(), "wincall delete_on_exit direct=on queue=5");
    STXXL_CHECK_EQUAL(cfg.queue, 5);
    STXXL_CHECK_EQUAL(cfg.direct, foxxll::disk_config::DIRECT_ON);

    // bad configurations

    STXXL_CHECK_THROW(
        cfg.parse_line("disk=/var/tmp/foxxll.tmp, 100 GiB, wincall_fileperblock unlink direct=on"),
        std::runtime_error
        );

    STXXL_CHECK_THROW(
        cfg.parse_line("disk=/var/tmp/foxxll.tmp,0x,syscall"),
        std::runtime_error
        );
}

void test2()
{
#if !STXXL_WINDOWS
    // test user-supplied configuration

    foxxll::config* config = foxxll::config::get_instance();

    {
        foxxll::disk_config disk1("/tmp/foxxll-1.tmp", 100 * 1024 * 1024,
                                  "syscall");
        disk1.unlink_on_open = true;
        disk1.direct = foxxll::disk_config::DIRECT_OFF;

        STXXL_CHECK_EQUAL(disk1.path, "/tmp/foxxll-1.tmp");
        STXXL_CHECK_EQUAL(disk1.size, 100 * 1024 * uint64_t(1024));
        STXXL_CHECK_EQUAL(disk1.autogrow, 1);
        STXXL_CHECK_EQUAL(disk1.fileio_string(),
                          "syscall direct=off unlink_on_open");

        config->add_disk(disk1);

        foxxll::disk_config disk2("/tmp/foxxll-2.tmp", 200 * 1024 * 1024,
                                  "syscall autogrow=no direct=off");
        disk2.unlink_on_open = true;

        STXXL_CHECK_EQUAL(disk2.path, "/tmp/foxxll-2.tmp");
        STXXL_CHECK_EQUAL(disk2.size, 200 * 1024 * uint64_t(1024));
        STXXL_CHECK_EQUAL(disk2.fileio_string(),
                          "syscall autogrow=no direct=off unlink_on_open");
        STXXL_CHECK_EQUAL(disk2.direct, 0);

        config->add_disk(disk2);
    }

    STXXL_CHECK_EQUAL(config->disks_number(), 2);
    STXXL_CHECK_EQUAL(config->total_size(), 300 * 1024 * 1024);

    // construct block_manager with user-supplied config

    foxxll::block_manager* bm = foxxll::block_manager::get_instance();

    STXXL_CHECK_EQUAL(bm->total_bytes(), 300 * 1024 * 1024);
    STXXL_CHECK_EQUAL(bm->free_bytes(), 300 * 1024 * 1024);

#endif
}

int main()
{
    test1();
    test2();

    return 0;
}
