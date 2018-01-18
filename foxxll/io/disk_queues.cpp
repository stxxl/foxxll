/***************************************************************************
 *  foxxll/io/disk_queues.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <foxxll/io/disk_queues.hpp>

#include <foxxll/io/iostats.hpp>
#include <foxxll/io/linuxaio_queue.hpp>
#include <foxxll/io/linuxaio_request.hpp>
#include <foxxll/io/request_queue_impl_qwqr.hpp>
#include <foxxll/io/serving_request.hpp>

namespace foxxll {

disk_queues::disk_queues()
{
    stats::get_instance();     // initialize stats before ourselves
}

disk_queues::~disk_queues()
{
    std::unique_lock<std::mutex> lock(mutex_);
    // deallocate all queues_
    for (request_queue_map::iterator i = queues_.begin(); i != queues_.end(); i++)
        delete (*i).second;
}

void disk_queues::make_queue(file* file)
{
    std::unique_lock<std::mutex> lock(mutex_);

    int queue_id = file->get_queue_id();

    request_queue_map::iterator qi = queues_.find(queue_id);
    if (qi != queues_.end())
        return;

    // create new request queue
#if STXXL_HAVE_LINUXAIO_FILE
    if (const linuxaio_file* af =
            dynamic_cast<const linuxaio_file*>(file)) {
        queues_[queue_id] = new linuxaio_queue(af->get_desired_queue_length());
        return;
    }
#endif
    queues_[queue_id] = new request_queue_impl_qwqr();
}

void disk_queues::add_request(request_ptr& req, disk_id_type disk)
{
    std::unique_lock<std::mutex> lock(mutex_);

#ifdef STXXL_HACK_SINGLE_IO_THREAD
    disk = 42;
#endif
    request_queue_map::iterator qi = queues_.find(disk);
    request_queue* q;
    if (qi == queues_.end())
    {
        // create new request queue
#if STXXL_HAVE_LINUXAIO_FILE
        if (dynamic_cast<linuxaio_request*>(req.get()))
            q = queues_[disk] = new linuxaio_queue(
                    dynamic_cast<linuxaio_file*>(req->get_file())->get_desired_queue_length());
        else
#endif
        q = queues_[disk] = new request_queue_impl_qwqr();
    }
    else
        q = qi->second;

    q->add_request(req);
}

bool disk_queues::cancel_request(request_ptr& req, disk_id_type disk)
{
    std::unique_lock<std::mutex> lock(mutex_);

#ifdef STXXL_HACK_SINGLE_IO_THREAD
    disk = 42;
#endif
    if (queues_.find(disk) != queues_.end())
        return queues_[disk]->cancel_request(req);
    else
        return false;
}

request_queue* disk_queues::get_queue(disk_id_type disk)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (queues_.find(disk) != queues_.end())
        return queues_[disk];
    else
        return nullptr;
}

void disk_queues::set_priority_op(const request_queue::priority_op& op)
{
    std::unique_lock<std::mutex> lock(mutex_);

    for (request_queue_map::iterator i = queues_.begin(); i != queues_.end(); i++)
        i->second->set_priority_op(op);
}

} // namespace foxxll
// vim: et:ts=4:sw=4
