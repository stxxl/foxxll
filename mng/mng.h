#ifndef MNG_HEADER
#define MNG_HEADER
/***************************************************************************
 *            mng.h
 *
 *  Sat Aug 24 23:55:27 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "../io/io.h"
#include "../common/rand.h"
#include "../common/aligned_alloc.h"


#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <stdlib.h>


__STXXL_BEGIN_NAMESPACE

  //! \defgroup mnglayer Block management layer
  //! Group of classes which help controlling external memory space,
  //! managing disks, and allocating and deallocating blocks of external storage
  //! \{

	//! \brief Block identifier class
	
	//! Stores block identity, given by file and offset within the file
	template < unsigned SIZE > 
  struct BID
	{
		enum 
		{
			size = SIZE  //!< Block size
		};
		file * storage; //!< pointer to the file of the block
		off_t offset; //!< offset within the file of the block
    BID():storage(NULL),offset(0) {}
    bool valid() const { return storage; }
	};

	
	template <unsigned bytes>
	class filler_struct__
	{
		typedef unsigned char byte_type;
		byte_type filler_array_[bytes];
	};
	
	template <>
	class filler_struct__<0>
	{
		typedef unsigned char byte_type;
	};

		
	template < unsigned SIZE, class T, unsigned REF_NUM, bool NEEDS_FILLER>
	class typed_block_base
	{
	};
	
	template < unsigned SIZE, class T, unsigned REF_NUM>
	class typed_block_base<SIZE,T,REF_NUM,false>
	{
	 public:

		BID<SIZE> ref[REF_NUM];
		
		BID<SIZE> & operator ()(int i)
		{
			return ref[i];
		};
	};

	
	template < unsigned SIZE, class T > 
	class typed_block_base < SIZE, T, 0,false >
	{
	};

	// one has to add filler in order to make sizeof(typed_block) == SIZE
	template < unsigned SIZE, class T, unsigned REF_NUM>
	class typed_block_base<SIZE,T,REF_NUM,true>
	{
		filler_struct__<SIZE-((SIZE-sizeof(BID<SIZE>)*REF_NUM)/sizeof(T))*sizeof(T)> __filler;
	 public:
		BID < SIZE > ref[REF_NUM];

		BID < SIZE > & operator ()(int i)
		{
			return ref[i];
		};
	};

	// one has to add filler in order to make sizeof(typed_block) == SIZE
	template < unsigned SIZE, class T > 
	class typed_block_base < SIZE, T, 0,true >
	{
    filler_struct__<SIZE - (SIZE / sizeof (T))*sizeof (T)> __filler;
	};
	
	//! \brief Block containing elements of fixed length
	template < unsigned SIZE, class T, unsigned REF_NUM = 0>
	class typed_block: public typed_block_base<SIZE,T,REF_NUM,SIZE % sizeof(T)>
	{
	public:
		typedef T type;
		typedef T value_type;
		typedef T & reference;
		typedef const T & const_reference;
		typedef type * pointer;
		typedef pointer iterator;
		typedef const pointer const_iterator;
		
		enum { has_filler = SIZE % sizeof(T) };
		
		typedef BID<SIZE> bid_type;
		
		//! Array of elements of type T
		T elem[(SIZE - sizeof(BID < SIZE >) * REF_NUM) / sizeof (T)];
		
		typed_block() {};
	
		//! An operator to access elements in the block
		reference operator [](int i)
		{
			return elem[i];
		};
		
		enum
		{ 
			raw_size = SIZE, //!< size of block in bytes
			size = (SIZE - sizeof (BID < SIZE >) * REF_NUM) / sizeof (T) //!< number of elements in block
		};

		//! \brief Returns \c iterator pointing to the first element
		iterator begin()
		{
			return elem;
		};
		//! \brief Returns \c const_iterator pointing to the first element
		const_iterator begin() const
		{
			return elem;
		};
		//! \brief Returns \c iterator pointing to the end element
		iterator end()
		{
			return elem + size;
		};
		//! \brief Returns \c const_iterator pointing to the end element
		const_iterator end() const
		{
			return elem + size;
		};
		
		//! \brief Writes block to the disk(s)
		//! \param bid block identifier, points the file(disk) and position
		//! \param on_cmpl completion handler
    //! \return \c pointer_ptr object to track status I/O operation after the call
		request_ptr write (const BID<SIZE> & bid, 
			    completion_handler on_cmpl = default_completion_handler())
		{
			return bid.storage->awrite(
						this, 
						bid.offset, 
						SIZE, 
					  on_cmpl);
		};

		//! \brief Reads block from the disk(s)
		//! \param bid block identifier, points the file(disk) and position
		//! \param on_cmpl completion handler
    //! \return \c pointer_ptr object to track status I/O operation after the call
		request_ptr read (const BID < SIZE > &bid,
			   completion_handler on_cmpl = default_completion_handler())
		{
			return bid.storage->aread(this, bid.offset, SIZE, on_cmpl);
		};
		
		void *operator      new[] (size_t bytes)
		{
			return aligned_alloc < BLOCK_ALIGN > (bytes);
		}
		void *operator      new (size_t bytes)
		{
			return aligned_alloc < BLOCK_ALIGN > (bytes);
		}
		void operator      delete (void *ptr)
		{
			aligned_dealloc < BLOCK_ALIGN > (ptr);
		}
		void operator      delete[] (void *ptr)
		{
			aligned_dealloc < BLOCK_ALIGN > (ptr);
		}
	};
	

	
/*
template <unsigned BLK_SIZE>
class BIDArray: public std::vector< BID <BLK_SIZE> >
{
 public:
  BIDArray(std::vector< BID <BLK_SIZE> >::size_type size = 0) : std::vector< BID <BLK_SIZE> >(size) {};
};
*/

	template<unsigned BLK_SIZE> class BIDArray
	{
	protected:
		unsigned _size;
		BID < BLK_SIZE > *array;
	public:
		typedef BID<BLK_SIZE> & reference;
		typedef BID<BLK_SIZE> * iterator;
		typedef const BID<BLK_SIZE> * const_iterator;
		BIDArray ():_size (0), array (NULL)
		{
		};
		iterator begin ()
		{
			return array;
		};
		iterator end ()
		{
			return array + _size;
		};
		
	  BIDArray (unsigned size):_size (size)
		{
			array = new BID < BLK_SIZE >[size];
		};
		unsigned size ()
		{
			return _size;
		};
		reference operator [](int i)
		{
			return array[i];
		};
		void resize (unsigned newsize)
		{
			if (array)
			{
				stxxl_debug (std::cerr <<
					     "Warning: resizing nonempty BIDArray"
					     << std::endl;)
				BID < BLK_SIZE > *tmp = array;
				array = new BID < BLK_SIZE >[newsize];
				memcpy (array, tmp, sizeof (BID <BLK_SIZE >) * STXXL_MIN (_size, newsize));
				delete [] tmp;
				_size = newsize;
			}
			else
			{
				array = new BID < BLK_SIZE >[newsize];
				_size = newsize;
			}
		};
		~BIDArray ()
		{
			if (array)
				delete[]array;
		};
	};


	class DiskAllocator
	{
		typedef std::pair < off_t, off_t > place;
		struct FirstFit:public std::binary_function < place, off_t,bool >
		{
			bool operator     () (
								const place & entry,
					      const off_t size) const
			{
				return (entry.second >= size);
			}
		};
		struct OffCmp
		{
			bool operator      () (const off_t & off1,const off_t & off2)
			{
				return off1 < off2;
			};
		};

		DiskAllocator ()
		{
		};
	protected:
		
		typedef std::map < off_t, off_t > sortseq;
		sortseq free_space;
		//  sortseq used_space;
		off_t free_bytes;
		off_t disk_bytes;
	
  public:
		DiskAllocator (off_t disk_size);

		off_t get_free_bytes () const
		{
			return free_bytes;
		};
		off_t get_used_bytes () const
		{
			return disk_bytes - free_bytes;
		};

		template < unsigned BLK_SIZE >
		void new_blocks (BIDArray < BLK_SIZE > &bids);
		
		template < unsigned BLK_SIZE >
		void delete_blocks (const BIDArray < BLK_SIZE > &bids);
		template < unsigned BLK_SIZE > 
		void delete_block (const BID <BLK_SIZE > & bid);
	};

  DiskAllocator::DiskAllocator (off_t disk_size):
		free_bytes(disk_size),
		disk_bytes(disk_size)
	{
		free_space[0] = disk_size;
	}

	template < unsigned BLK_SIZE >
		void DiskAllocator::new_blocks (BIDArray < BLK_SIZE > &bids)
	{
		off_t requested_size = off_t(bids.size()) * off_t(BLK_SIZE);
		sortseq::iterator space = 
			std::find_if (free_space.begin (), free_space.end (),
				      bind2nd(FirstFit (), requested_size));

		if (space != free_space.end ())
		{
			off_t region_pos = (*space).first;
			off_t region_size = (*space).second;
			free_space.erase (space);
			if (region_size > requested_size)
				free_space[region_pos + requested_size] = region_size - requested_size;

			for (unsigned i = 0; i < bids.size (); i++)
			{
				bids[i].offset =
					region_pos +
					off_t (i) * off_t (BLK_SIZE);
			}
			free_bytes -= requested_size;
		}
		else
		{
			STXXL_ERRMSG( "Allocation error: " << requested_size <<
				" bytes requested, " << free_bytes <<
				" bytes free" )
			abort();
		}
	};

	template < unsigned BLK_SIZE >
		void DiskAllocator::delete_block (const BID < BLK_SIZE > &bid)
	{
		off_t region_pos = bid.offset;
		off_t region_size = BLK_SIZE;
		sortseq::iterator succ = free_space.upper_bound (region_pos);
		sortseq::iterator pred = succ;
		pred--;
		if (succ != free_space.end ()
		    && (*succ).first == region_pos + region_size)
		{
			// coalesce with successor
			region_size += (*succ).second;
			free_space.erase (succ);
		}
		if (pred != free_space.end ()
		    && (*pred).first + (*pred).second == region_pos)
		{
			// coalesce with predecessor
			region_size += (*pred).second;
			region_pos = (*pred).first;
			free_space.erase (pred);
		}

		free_space[region_pos] = region_size;
		free_bytes += off_t (BLK_SIZE);
	}

	template < unsigned BLK_SIZE >
		void DiskAllocator::delete_blocks (const BIDArray < BLK_SIZE > &bids)
	{
		//  STXXL_MSG("delete free:" << free_bytes << " total:"<< disk_bytes )
		for (int i = 0; i < bids.size (); i++)
		{
			off_t region_pos = bids[i].offset;
			off_t region_size = BLK_SIZE;
			sortseq::iterator succ =
				free_space.upper_bound (region_pos);
			sortseq::iterator pred = succ;
			pred--;

			if (succ != free_space.end ()
			    && (*succ).first == region_pos + region_size)
			{
				// coalesce with successor

				region_size += (*succ).second;
				free_space.erase (succ);
			}
			if (pred != free_space.end ()
			    && (*pred).first + (*pred).second == region_pos)
			{
				// coalesce with predecessor

				region_size += (*pred).second;
				region_pos = (*pred).first;
				free_space.erase (pred);
			}

			free_space[region_pos] = region_size;
		};
		free_bytes += off_t (bids.size ()) * off_t (BLK_SIZE);
	}

	//! \brief Access point to disks properties
	//! \remarks is a singleton
	class config
	{
		struct DiskEntry
		{
			std::string path;
			std::string io_impl;
			off_t size;
		};
		std::vector < DiskEntry > disks_props;
		
		config (const char *config_path = "./.stxxl");
	 public:
		//! \brief Returns number of disks available to user
		//! \return number of disks
		unsigned disks_number()
		{
			return disks_props.size ();
		};
		
		unsigned ndisks()
		{
			return disks_props.size ();
		};
		
		//! \brief Returns path of disks
		//! \param disk disk's identifier
		//! \return string that contains the disk's path name
		const std::string & disk_path (int disk)
		{
			return disks_props[disk].path;
		};
		//! \brief Returns disk size
		//! \param disk disk's identifier
		//! \return disk size in bytes
		off_t disk_size (int disk)
		{
			return disks_props[disk].size;
		};
		//! \brief Returns name of I/O implementation of particular disk
		//! \param disk disk's identifier
		const std::string & disk_io_impl (int disk)
		{
			return disks_props[disk].io_impl;
		};

		//! \brief Returns instance of config
		//! \return pointer to the instance of config
		static config *get_instance ();
	private:
		static config *instance;
	};


	config *config::get_instance ()
	{
		if (!instance)
		{
			char *cfg_path = getenv ("STXXLCFG");
			if (cfg_path)
				instance = new config (cfg_path);
			else
				instance = new config ();
		}

		return instance;
	}


	config::config (const char *config_path)
	{
		std::ifstream cfg_file (config_path);
		if (!cfg_file)
		{
			STXXL_ERRMSG("Warning: no config file found." )
			STXXL_ERRMSG("Using default disk configuration." )
			DiskEntry entry1 = { "/var/tmp/stxxl0", "syscall",
				100 * 1024 * 1024
			};
			DiskEntry entry2 =
				{ "/tmp/stxxl1", "mmap", 100 * 1024 * 1024 };
			DiskEntry entry3 = { "/tmp/stxxl2", "simdisk",
				100 * 1024 * 1024
			};
			disks_props.push_back (entry1);
			disks_props.push_back (entry2);
			disks_props.push_back (entry3);
		}
		else
		{
			std::string line;

			while (cfg_file >> line)
			{
				std::vector < std::string > tmp = split (line, "=");
				
				if(tmp[0][0] == '#')
				{
				}
				else
				if (tmp[0] == "disk")
				{
					tmp = split (tmp[1], ",");
					DiskEntry entry = { tmp[0], tmp[2],
							off_t (str2int (tmp[1])) *
							off_t (1024 * 1024)
					};
					disks_props.push_back (entry);
				}
				else
				{
					std::cerr << "Unknown token " <<
						tmp[0] << std::endl;
				}
			}
			cfg_file.close ();
		}

		if (disks_props.empty ())
		{
			STXXL_ERRMSG( "No disks found in '" << config_path << "' ." )
			abort ();
		}
		else
		{
			for (std::vector < DiskEntry >::const_iterator it =
			     disks_props.begin (); it != disks_props.end ();
			     it++)
			{
				STXXL_MSG("Disk '" << (*it).path << "' is allocated, space: " <<
					((*it).size) / (1024 * 1024) <<
					" Mb, I/O implementation: " << (*it).io_impl )
			}
		}
	};

	class FileCreator
	{
	public:
		virtual stxxl::file * create (const std::string & io_impl,
					      const std::string & filename,
					      int options, int disk)
		{
			if (io_impl == "syscall")
				return new stxxl::syscall_file (filename,
								options,
								disk);
			else if (io_impl == "mmap")
				return new stxxl::mmap_file (filename,
							     options, disk);
			else if (io_impl == "simdisk")
				return new stxxl::sim_disk_file (filename,
								 options,
								 disk);

			STXXL_ERRMSG("Unsupported disk I/O implementation " <<
				io_impl << " ." )
			abort ();

			return NULL;
		};
	};

	//! \brief striping disk allocation scheme functor
	//! \remarks model of \b allocation_strategy concept
	struct striping
	{
		int begin, diff;
		striping (int b, int e):begin (b), diff (e - b)
		{
		};
	      striping ():begin (0)
		{
			diff = config::get_instance ()->disks_number ();
		};
		int operator     () (int i) const
		{
			return begin + i % diff;
		};
		static const char * name()
		{
			return "striping";
		}
	};

	//! \brief fully randomized disk allocation scheme functor
	//! \remarks model of \b allocation_strategy concept
	struct FR:public striping
	{
		random_number<random_uniform_fast> rnd;
		FR (int b, int e):striping (b, e)
		{
		};
	  FR ():striping ()
		{
		};
		int operator     () (int i) const
		{
			return begin + rnd(diff);
		}
		static const char * name()
		{
			return "fully randomized striping";
		}
	};

	//! \brief simple randomized disk allocation scheme functor
	//! \remarks model of \b allocation_strategy concept
	struct SR:public striping
	{
		random_number<random_uniform_fast> rnd;
		int offset;
		SR (int b, int e):striping (b, e)
		{
			offset = rnd(diff);
		};
		SR():striping ()
		{
			offset = rnd(diff);
		};
		int operator     () (int i) const
		{
			return begin + (i + offset) % diff;
		}
		static const char * name()
		{
			return "simple randomized striping";
		}
	};

	//! \brief randomized cycling disk allocation scheme functor
	//! \remarks model of \b allocation_strategy concept
	struct RC:public striping
	{
		std::vector<int> perm;
		 
		RC (int b, int e):striping (b, e), perm (diff)
		{
			for (int i = 0; i < diff; i++)
				perm[i] = i;

			stxxl::random_number<random_uniform_fast> rnd;
			std::random_shuffle (perm.begin (), perm.end (), rnd);
		}
		RC ():striping (), perm (diff)
		{
			for (int i = 0; i < diff; i++)
				perm[i] = i;

			random_number<random_uniform_fast> rnd;
			std::random_shuffle (perm.begin (), perm.end (), rnd);
		}
		int operator     () (int i) const
		{
			return begin + perm[i % diff];
		}
		static const char * name()
		{
			return "randomized cycling striping";
		}
	};

	//! \brief 'single disk' disk allocation scheme functor
	//! \remarks model of \b allocation_strategy concept
	struct single_disk
	{
		const int disk;
		single_disk(int d):disk(d)
		{
		};
	  
    single_disk():disk(0)
		{
		};
		int operator() (int i) const
		{
			return disk;
		};
		static const char * name()
		{
			return "single disk";
		}
	};
  
  template <class BaseAllocator_>
  struct offset_allocator
  {
    BaseAllocator_ base;
    int offset;
    offset_allocator(int offset_) : base(),offset(offset_) {}
    offset_allocator(int offset_,BaseAllocator_ & base_) : base(base_),offset(offset_) {}
    int operator() (int i)
    {
      return base(offset + i);
    }
  };

	//! \brief Traits for models of \b bid_iterator concept
	template < class bid_it > struct bid_iterator_traits
	{
		bid_it *a;
		enum
		{
			block_size = bid_it::block_size
		};
	};

	template < unsigned blk_sz > struct bid_iterator_traits <BID <blk_sz > *>
	{
		enum
		{
			block_size = blk_sz
		};
	};

/*
	template < unsigned _blk_sz > struct bid_iterator_traits<
		std::__normal_iterator< BID<_blk_sz> * ,std::vector< BID<_blk_sz> ,std::allocator<BID<_blk_sz> > >  > >
			 
	{
		enum
		{
			block_size = _blk_sz
		};	
	};
*/

	template < unsigned _blk_sz > struct bid_iterator_traits<
		__gnu_cxx::__normal_iterator< BID<_blk_sz> * ,std::vector< BID<_blk_sz> ,std::allocator<BID<_blk_sz> > >  > >
	{
		enum
		{
			block_size = _blk_sz
		};	
	};	

	//! \brief Block manager class
	
	//! Manages allocation and deallocation of blocks in multiple/single disk setting
	//! \remarks is a singleton
	class block_manager
	{
		DiskAllocator **disk_allocators;
		file ** disk_files;
		
		unsigned ndisks;
		block_manager ();
	
	public:
		//! \brief Returns instance of block_manager
		//! \return pointer to the only instance of block_manager
		static block_manager *get_instance ();

		//! \brief Allocates new blocks
		
		//! Allocates new blocks according to the strategy 
		//! given by \b functor and stores block identifiers
		//! to the range [ \b bidbegin, \b bidend)
		//! \param functor object of model of \b allocation_strategy concept
		//! \param bidbegin iterator object of \b bid_iterator concept
		//! \param bidend iterator object of \b bid_iterator concept
		template < class DiskAssgnFunctor, class BIDIteratorClass >
		void new_blocks (
					 DiskAssgnFunctor functor,
					 const BIDIteratorClass & bidbegin,
					 const BIDIteratorClass & bidend);

		//! \brief Deallocates blocks
		
		//! Deallocates blocks in the range [ \b bidbegin, \b bidend)
		//! \param bidbegin iterator object of \b bid_iterator concept
		//! \param bidend iterator object of \b bid_iterator concept
		template < class BIDIteratorClass >
		void delete_blocks (const BIDIteratorClass & bidbegin, const BIDIteratorClass & bidend);

		//! \brief Deallocates a block
		//! \param bid block identifier
		template < unsigned BLK_SIZE >
	  void delete_block (const BID < BLK_SIZE > &bid);

		~block_manager ();
	private:
		static block_manager *instance;
	};

	block_manager *block_manager::get_instance ()
	{
		if (!instance)
			instance = new block_manager ();

		return instance;
	}

	block_manager::block_manager ()
	{
		FileCreator fc;
		config *cfg = config::get_instance ();

		ndisks = cfg->disks_number ();
		disk_allocators = new DiskAllocator *[ndisks];
		disk_files = new stxxl::file *[ndisks];

		for (unsigned i = 0; i < ndisks; i++)
		{
			disk_files[i] = fc.create (cfg->disk_io_impl (i),
						   cfg->disk_path (i),
						   stxxl::file::CREAT | stxxl::file::RDWR  | stxxl::file::DIRECT
						   ,i);
			disk_files[i]->set_size (cfg->disk_size (i));
			disk_allocators[i] = new DiskAllocator (cfg->disk_size (i));
		}
	};

	block_manager::~block_manager()
	{
		for (unsigned i = 0; i < ndisks; i++)
		{
			delete disk_allocators[i];
			delete disk_files[i];
		}
		delete[]disk_allocators;
		delete[]disk_files;
	}


	template < class DiskAssgnFunctor, class BIDIteratorClass >
		void block_manager::new_blocks (
						DiskAssgnFunctor functor,
						const BIDIteratorClass & bidbegin,
						const BIDIteratorClass & bidend)
	{
		unsigned nblocks = bidend - bidbegin;

		int *bl = new int[ndisks];
		BIDArray < bid_iterator_traits <BIDIteratorClass >::block_size > *disk_bids =
			new BIDArray < bid_iterator_traits <BIDIteratorClass >::block_size >[ndisks];

		memset(bl, 0, ndisks * sizeof (int));

		unsigned i = 0;
		BIDIteratorClass it = bidbegin;
		for (; i < nblocks; i++, it++)
		{
			int disk = functor (i);
			(*it).storage = disk_files[disk];
			bl[disk]++;
		}

		for (i = 0; i < ndisks; i++)
		{
			if (bl[i])
			{
				disk_bids[i].resize (bl[i]);
				disk_allocators[i]->new_blocks (disk_bids[i]);
			}
		}

		memset (bl, 0, ndisks * sizeof (int));

		i = 0;
		for (it = bidbegin; it != bidend; it++, i++)
		{
			int disk = (*it).storage->get_disk_number ();
			//    std::cout << disk_bids[disk][bl[disk]].offset << " "<<std::endl;
			(*it).offset = disk_bids[disk][bl[disk]++].offset;
			//    std::cout <<   (*it).offset << " "<<std::endl;
		}

		delete[]bl;
		delete[]disk_bids;
	}



	template < unsigned BLK_SIZE >
	void block_manager::delete_block (const BID < BLK_SIZE > &bid)
	{
		disk_allocators[bid.storage->get_disk_number ()]->delete_block (bid);
	}


	template < class BIDIteratorClass >
		void block_manager::delete_blocks (
						const BIDIteratorClass & bidbegin,
			      const BIDIteratorClass & bidend)
	{
		for (BIDIteratorClass it = bidbegin; it != bidend; it++)
		{
			delete_block (*it);
		}
	}

	block_manager *block_manager::instance = NULL;
	config *config::instance = NULL;

  #define STXXL_DEFAULT_ALLOC_STRATEGY RC   
  #define STXXL_DEFAULT_BLOCK_SIZE(type) (2*1024*1024)	// use traits
  
  //! \}
  
__STXXL_END_NAMESPACE

#endif
