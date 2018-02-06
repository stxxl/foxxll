/***************************************************************************
 *  foxxll/mng/disk_block_allocator.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013-2015 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_MNG_DISK_BLOCK_ALLOCATOR_HEADER
#define FOXXLL_MNG_DISK_BLOCK_ALLOCATOR_HEADER

#include <algorithm>
#include <cassert>
#include <map>
#include <mutex>
#include <ostream>
#include <utility>

#include <tlx/logger.hpp>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/common/exceptions.hpp>
#include <foxxll/common/types.hpp>
#include <foxxll/io/file.hpp>
#include <foxxll/mng/bid.hpp>
#include <foxxll/mng/config.hpp>

namespace foxxll {

//! \ingroup mnglayer
//! \{

/*!
 * This class manages allocation of blocks onto a single disk. It contains a map
 * of all currently allocated blocks. The block_manager selects which of the
 * disk_block_allocator objects blocks are drawn from.
 */
class disk_block_allocator
{
    constexpr static bool debug = false;

public:
    disk_block_allocator(file* storage, const disk_config& cfg)
        : cfg_bytes_(cfg.size),
          storage_(storage),
          autogrow_(cfg.autogrow)
    {
        // initial growth to configured file size
        grow_file(cfg.size);
    }

    //! non-copyable: delete copy-constructor
    disk_block_allocator(const disk_block_allocator&) = delete;
    //! non-copyable: delete assignment operator
    disk_block_allocator& operator = (const disk_block_allocator&) = delete;

    ~disk_block_allocator()
    {
        if (disk_bytes_ > cfg_bytes_) { // reduce to original size
            storage_->set_size(cfg_bytes_);
        }
    }

    //! Returns autogrow
    bool autogrow() const { return autogrow_; }

    bool has_available_space(uint64_t bytes) const
    {
        return autogrow_ || free_bytes_ >= bytes;
    }

    uint64_t free_bytes() const
    {
        return free_bytes_;
    }

    uint64_t used_bytes() const
    {
        return disk_bytes_ - free_bytes_;
    }

    uint64_t total_bytes() const
    {
        return disk_bytes_;
    }

    template <size_t BlockSize>
    void new_blocks(BIDArray<BlockSize>& bids)
    {
        new_blocks(bids.begin(), bids.end());
    }

    template <typename BIDIterator>
    void new_blocks(BIDIterator begin, BIDIterator end);

    template <size_t BlockSize>
    void delete_blocks(const BIDArray<BlockSize>& bids)
    {
        for (size_t i = 0; i < bids.size(); ++i)
            delete_block(bids[i]);
    }

    template <size_t BlockSize>
    void delete_block(const BID<BlockSize>& bid)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        LOG << "disk_block_allocator::delete_block<" << BlockSize <<
            ">(pos=" << bid.offset << ", size=" << bid.size <<
            "), free:" << free_bytes_ << " total:" << disk_bytes_;

        add_free_region(bid.offset, bid.size);
    }

private:
    //! pair (offset, size) used for free space calculation
    using place = std::pair<uint64_t, uint64_t>;
    using space_map_type = std::map<uint64_t, uint64_t>;

    std::mutex mutex_;
    //! map of free space as places
    space_map_type free_space_;
    uint64_t free_bytes_ = 0;
    uint64_t disk_bytes_ = 0;
    uint64_t cfg_bytes_;
    file* storage_;
    bool autogrow_;

    void dump() const;

    void deallocation_error(
        uint64_t block_pos, uint64_t block_size,
        const space_map_type::iterator& pred,
        const space_map_type::iterator& succ) const;

    // expects the mutex_ to be locked to prevent concurrent access
    void add_free_region(uint64_t block_pos, uint64_t block_size);

    // expects the mutex_ to be locked to prevent concurrent access
    void grow_file(uint64_t extend_bytes)
    {
        if (extend_bytes == 0)
            return;

        storage_->set_size(disk_bytes_ + extend_bytes);
        add_free_region(disk_bytes_, extend_bytes);
        disk_bytes_ += extend_bytes;
    }
};

template <typename BIDIterator>
void disk_block_allocator::new_blocks(BIDIterator begin, BIDIterator end)
{
    uint64_t requested_size = 0;

    for (BIDIterator cur = begin; cur != end; ++cur)
    {
        LOG << "Asking for a block with size: " << cur->size;
        requested_size += cur->size;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    LOG << "disk_block_allocator::new_blocks<BlockSize>"
        ", BlockSize = " << begin->size <<
        ", free:" << free_bytes_ << " total:" << disk_bytes_ <<
        ", blocks: " << (end - begin) <<
        ", requested_size=" << requested_size;

    if (free_bytes_ < requested_size)
    {
        if (!autogrow_) {
            FOXXLL_THROW(bad_ext_alloc,
                         "Out of external memory error: " << requested_size <<
                         " requested, " << free_bytes_ << " bytes free. "
                         "Maybe enable autogrow_ flags?");
        }

        LOG1 << "External memory block allocation error: " << requested_size <<
            " bytes requested, " << free_bytes_ <<
            " bytes free. Trying to extend the external memory space...";

        grow_file(requested_size);
    }

    // dump();

    space_map_type::iterator space =
        std::find_if(free_space_.begin(), free_space_.end(),
                     [requested_size](const place& entry) {
                         return (entry.second >= requested_size);
                     });

    if (space == free_space_.end() && begin + 1 == end)
    {
        if (!autogrow_) {
            LOG1 << "Warning: Severe external memory space fragmentation!";
            dump();

            LOG1 << "External memory block allocation error: " << requested_size <<
                " bytes requested, " << free_bytes_ <<
                " bytes free. Trying to extend the external memory space...";
        }

        grow_file(begin->size);

        space = std::find_if(free_space_.begin(), free_space_.end(),
                             [requested_size](const place& entry) {
                                 return (entry.second >= requested_size);
                             });
    }

    if (space != free_space_.end())
    {
        uint64_t region_pos = (*space).first;
        uint64_t region_size = (*space).second;
        free_space_.erase(space);

        if (region_size > requested_size)
            free_space_[region_pos + requested_size] = region_size - requested_size;

        for (uint64_t pos = region_pos; begin != end; ++begin)
        {
            begin->offset = pos;
            pos += begin->size;
        }

        assert(free_bytes_ >= requested_size);
        free_bytes_ -= requested_size;
        //dump();

        return;
    }

    // no contiguous region found
    LOG << "Warning, when allocating an external memory space, no"
        "contiguous region found. It might harm the performance";

    assert(end - begin > 1);

    lock.unlock();

    BIDIterator middle = begin + ((end - begin) / 2);
    new_blocks(begin, middle);
    new_blocks(middle, end);
}

//! \}

} // namespace foxxll

#endif // !FOXXLL_MNG_DISK_BLOCK_ALLOCATOR_HEADER
