/***************************************************************************
 *  foxxll/common/tmeta.hpp
 *
 *  Template Metaprogramming Tools
 *  (from the Generative Programming book Krysztof Czarnecki, Ulrich Eisenecker)
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_TMETA_HEADER
#define STXXL_COMMON_TMETA_HEADER

#include <foxxll/common/types.hpp>
#include <tlx/meta/if.hpp>

namespace foxxll {

const int DEFAULT = ~(~0u >> 1); // initialize with the smallest int

struct NilCase { };

template <int Tag, class Type_, class Next_ = NilCase>
struct CASE
{
    enum { tag = Tag };
    using Type = Type_;
    using Next = Next_;
};

template <int Tag, class Case>
class SWITCH
{
    using NextCase = typename Case::Next;
    enum
    {
        caseTag = Case::tag,
        found = (caseTag == Tag || caseTag == DEFAULT)
    };

public:
    using type = typename tlx::If<found,
                               typename Case::Type,
                               typename SWITCH<Tag, NextCase>::type
                               >::type;
};

template <int Tag>
class SWITCH<Tag, NilCase>
{
public:
    using type = NilCase;
};

} // namespace foxxll

#endif // !STXXL_COMMON_TMETA_HEADER
