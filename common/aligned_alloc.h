#ifndef ALIGNED_ALLOC
#define ALIGNED_ALLOC

/***************************************************************************
 *            aligned_alloc.h
 *
 *  Sat Aug 24 23:53:12 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "../common/utils.h"


__STXXL_BEGIN_NAMESPACE 


template < size_t ALIGNMENT > 
inline void * aligned_alloc (size_t size)
{
	STXXL_VERBOSE2("stxxl::aligned_alloc<"<<ALIGNMENT <<">(), size = "<<size) 
    // TODO: use posix_memalign() and free() on Linux
	char *buffer = new char[size + ALIGNMENT];
	char *result = buffer + ALIGNMENT - (((unsigned long) buffer) % (ALIGNMENT));
	*(((char **) result) - 1) = buffer;
	STXXL_VERBOSE2("stxxl::aligned_alloc<"<<ALIGNMENT <<
		">(), allocated at "<<std::hex <<((unsigned long)buffer)<<" returning "<< ((unsigned long)result)
		<<std::dec) 
	//abort();
	
	return result;
};

template < size_t ALIGNMENT > inline void
aligned_dealloc (void *ptr)
{
	STXXL_VERBOSE2("stxxl::aligned_dealloc(<"<<ALIGNMENT <<">), ptr = "<<ptr) 
	delete[] * (((char **) ptr) - 1);
};

__STXXL_END_NAMESPACE
#endif
