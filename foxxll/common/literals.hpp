/***************************************************************************
 *  foxxll/common/literals.hpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2018 Manuel Penschuck <foxxll@manuel.jetzt>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_COMMON_LITERALS_HEADER
#define FOXXLL_COMMON_LITERALS_HEADER

#include <limits>

#include <foxxll/common/types.hpp>

// external sizes
constexpr foxxll::external_size_type operator""_eB  (unsigned long long int x) {return x;}
constexpr foxxll::external_size_type operator""_ekB (unsigned long long int x) {return x*1000u;}
constexpr foxxll::external_size_type operator""_eMB (unsigned long long int x) {return x*1000_ekB;}
constexpr foxxll::external_size_type operator""_eGB (unsigned long long int x) {return x*1000_eMB;}
constexpr foxxll::external_size_type operator""_eTB (unsigned long long int x) {return x*1000_eGB;}
constexpr foxxll::external_size_type operator""_ePB (unsigned long long int x) {return x*1000_eTB;}
constexpr foxxll::external_size_type operator""_eKiB(unsigned long long int x) {return x*1024u;}
constexpr foxxll::external_size_type operator""_eMiB(unsigned long long int x) {return x*1024_eKiB;}
constexpr foxxll::external_size_type operator""_eGiB(unsigned long long int x) {return x*1024_eMiB;}
constexpr foxxll::external_size_type operator""_eTiB(unsigned long long int x) {return x*1024_eGiB;}
constexpr foxxll::external_size_type operator""_ePiB(unsigned long long int x) {return x*1024_eTiB;}

// internal sizes
constexpr size_t operator""_iB  (unsigned long long int x) {return static_cast<size_t>(x*1_eB  );}
constexpr size_t operator""_ikB (unsigned long long int x) {return static_cast<size_t>(x*1_ekB );}
constexpr size_t operator""_iMB (unsigned long long int x) {return static_cast<size_t>(x*1_eMB );}
constexpr size_t operator""_iGB (unsigned long long int x) {return static_cast<size_t>(x*1_eGB );}
constexpr size_t operator""_iTB (unsigned long long int x) {return static_cast<size_t>(x*1_eTB );}
constexpr size_t operator""_iPB (unsigned long long int x) {return static_cast<size_t>(x*1_ePB );}
constexpr size_t operator""_iKiB(unsigned long long int x) {return static_cast<size_t>(x*1_eKiB);}
constexpr size_t operator""_iMiB(unsigned long long int x) {return static_cast<size_t>(x*1_eMiB);}
constexpr size_t operator""_iGiB(unsigned long long int x) {return static_cast<size_t>(x*1_eGiB);}
constexpr size_t operator""_iTiB(unsigned long long int x) {return static_cast<size_t>(x*1_eTiB);}
constexpr size_t operator""_iPiB(unsigned long long int x) {return static_cast<size_t>(x*1_ePiB);}


#endif // FOXXLL_COMMON_LITERALS_HEADER
