#ifndef BUFFERED_WRITER_HEADER
#define BUFFERED_WRITER_HEADER

/***************************************************************************
 *            buf_writer.h
 *
 *  Mon Dec 30 10:57:58 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include <vector>
#include "mng.h"
#include "../common/utils.h"

__STXXL_BEGIN_NAMESPACE


//! \weakgroup schedlayer Block scheduling sublayer
//! \ingroup mnglayer
//! Group of classes which help in scheduling 
//! sequences of read and write requests
//! via prefetching and buffered writing
//! \{


//! \brief Encapsulates asynchronous buffered block writing engine
//!
//! \c buffered_writer overlaps I/Os with filling of output buffer.
template <typename block_type>
class buffered_writer
{
	buffered_writer() {}
protected:
	typedef typename block_type::bid_type bid_type;
	
	const int nwriteblocks;
	block_type *write_buffers;
	bid_type * write_bids;
	request_ptr * write_reqs;
	const unsigned writebatchsize;

	std::vector<int> free_write_blocks;	// contains free write blocks
	std::vector<int> busy_write_blocks;	// blocks that are in writing, notice that if block is not in free_
																			// an not in busy then block is not yet filled

	struct batch_entry
	{
		off_t offset;
		int ibuffer;
		batch_entry (off_t o, int b):offset (o), ibuffer (b) {};
	};
	struct batch_entry_cmp
	{
		bool operator () (const batch_entry & a, const batch_entry & b) const
		{
			return (a.offset > b.offset);
		};
	};

	typedef std::priority_queue < batch_entry, std::vector < batch_entry >, batch_entry_cmp > batch_type;
	batch_type batch_write_blocks;	// sorted sequence of blocks to write
	
public:
	//! \brief Constructs an object
	//! \param write_buf_size number of write buffers to use
	//! \param write_batch_size number of blocks to accumulate in 
	//!        order to flush write requests (bulk buffered writing)
	buffered_writer(int write_buf_size, int write_batch_size):
		nwriteblocks ((write_buf_size > 2) ? write_buf_size : 2),
		writebatchsize(write_batch_size ? write_batch_size : 1)
	{
		write_buffers = new block_type[nwriteblocks];
		write_reqs = new request_ptr[nwriteblocks];
		write_bids = new bid_type[nwriteblocks];

		for (unsigned i = 0; i < nwriteblocks; i++)
			free_write_blocks.push_back (i);
		
		disk_queues::get_instance()->set_priority_op(disk_queue::WRITE);
	}
	//! \brief Returns free block from the internal buffer pool
	//! \return pointer to the block from the internal buffer pool
	block_type * get_free_block ()
	{
		int ibuffer;
		for (std::vector<int>::iterator it = busy_write_blocks.begin ();
		     it != busy_write_blocks.end (); it++)
		{
			if (write_reqs[ibuffer = (*it)]->poll())
			{
				busy_write_blocks.erase (it);
				free_write_blocks.push_back (ibuffer);

				break;
			}
		}
		if (UNLIKELY (free_write_blocks.empty ()))
		{
			int size = busy_write_blocks.size ();
			request_ptr * reqs = new request_ptr[size];
			int i = 0;
			for (; i < size; i++)
			{
				reqs[i] = write_reqs[busy_write_blocks[i]];
			}
			int completed = wait_any(reqs, size);
			int completed_global = busy_write_blocks[completed];
			delete [] reqs;
			busy_write_blocks.erase (busy_write_blocks.begin () + completed);

			return (write_buffers + completed_global);
		}
		ibuffer = free_write_blocks.back();
		free_write_blocks.pop_back();

		return (write_buffers + ibuffer);
	}
	//! \brief Submi/home/rdementi/ts block for writing
	//! \param filled_block pointer to the block
	//! \remark parameter \c filled_block must be value returned by \c get_free_block() or \c write() methods
	//! \param bid block identifier, a place to write data of the \c filled_block
	//! \return pointer to the new free block from the pool
	block_type * write (block_type * filled_block, const bid_type & bid)	// writes filled_block and returns a new block 
	{
		if(batch_write_blocks.size () >= writebatchsize)
		{
			// flush batch
			while (!batch_write_blocks.empty ())
			{
				int ibuffer = batch_write_blocks.top ().ibuffer;
				batch_write_blocks.pop ();

				write_reqs[ibuffer] = write_buffers[ibuffer].write (write_bids[ibuffer]);

				busy_write_blocks.push_back(ibuffer);
			}

		}
		//    STXXL_MSG("Adding write request to batch")
		
		int ibuffer = filled_block - write_buffers;
		write_bids[ibuffer] = bid;
		batch_write_blocks.push(batch_entry(bid.offset,ibuffer));

		return get_free_block (); 
	}
	//! \brief Flushes not yet written buffers and frees used memory
	~buffered_writer ()
	{
		int ibuffer;
		while (!batch_write_blocks.empty ())
		{
			ibuffer = batch_write_blocks.top ().ibuffer;
			batch_write_blocks.pop();

			write_reqs[ibuffer] = write_buffers[ibuffer].write(write_bids[ibuffer]);
			
			busy_write_blocks.push_back (ibuffer);
		}
		for (std::vector<int>::const_iterator it =
		     busy_write_blocks.begin ();
		     it != busy_write_blocks.end (); it++)
		{
			ibuffer = *it;
			write_reqs[ibuffer]->wait ();
		}

		delete [] write_buffers;
		delete [] write_reqs;
		delete [] write_bids;
	}
	
};

//! \}

__STXXL_END_NAMESPACE

#endif
