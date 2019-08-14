/***************************************************************************
 *  foxxll/singleton.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2008, 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_SINGLETON_HEADER
#define FOXXLL_SINGLETON_HEADER

#include <cstdlib>
#include <mutex>

#include <foxxll/common/exithandler.hpp>
#include <foxxll/common/types.hpp>

namespace foxxll {

template <typename InstanceType, bool destroy_on_exit = true>
class singleton
{
public:
    using instance_type = InstanceType;
    using instance_pointer = instance_type *;
    using volatile_instance_pointer = volatile instance_pointer;

public:
    singleton() = default;

    //! non-copyable: delete copy-constructor
    singleton(const singleton&) = delete;
    //! non-copyable: delete assignment operator
    singleton& operator = (const singleton&) = delete;

    //! return instance or create base instance if empty
    static instance_pointer get_instance()
    {
        if (!instance_)
            return create_instance<InstanceType>();

        return instance_;
    }

    static instance_type & get_ref()
    {
        if (!instance_)
            create_instance<InstanceType>();

        return *instance_;
    }

    //! create instance of SubInstanceType and move into singleton
    template <typename SubInstanceType>
    static instance_pointer create_instance();

    //! destroy singleton and mark as invalid
    static void destroy_instance();

private:
    //! singleton object instance
    static volatile_instance_pointer instance_;

    //! mutex to protect instance_
    static std::mutex singleton_mutex_;
};

template <typename InstanceType, bool destroy_on_exit>
template <typename SubInstanceType>
typename singleton<InstanceType, destroy_on_exit>::instance_pointer
singleton<InstanceType, destroy_on_exit>::create_instance()
{
    std::unique_lock<std::mutex> instance_lock(singleton_mutex_);
    if (!instance_) {
        instance_ = new SubInstanceType();
        if (destroy_on_exit)
            register_exit_handler(destroy_instance);
    }
    return instance_;
}

template <typename InstanceType, bool destroy_on_exit>
void singleton<InstanceType, destroy_on_exit>::destroy_instance()
{
    std::unique_lock<std::mutex> instance_lock(singleton_mutex_);
    instance_pointer old_instance = instance_;
    instance_ = reinterpret_cast<instance_pointer>(size_t(-1));     // bomb if used again
    delete old_instance;
}

template <typename InstanceType, bool destroy_on_exit>
typename singleton<InstanceType, destroy_on_exit>::volatile_instance_pointer
singleton<InstanceType, destroy_on_exit>::instance_ = nullptr;

template <typename InstanceType, bool destroy_on_exit>
std::mutex singleton<InstanceType, destroy_on_exit>::singleton_mutex_;

} // namespace foxxll

#endif // !FOXXLL_SINGLETON_HEADER

/**************************************************************************/
