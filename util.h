#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/type_traits.h"
#include "auxi/contiguous_iterator_to_ptr.h"


/** @file
* @brief Contains as_signed/as_unsigned, index_valid, ssize and more
*/

namespace oel
{

//! Passed val of integral or enumeration type T, returns val cast to the signed integer type corresponding to T
template< typename T >  OEL_ALWAYS_INLINE
constexpr std::make_signed_t<T>   as_signed(T val) noexcept    { return std::make_signed_t<T>(val); }
//! Passed val of integral or enumeration type T, returns val cast to the unsigned integer type corresponding to T
template< typename T >  OEL_ALWAYS_INLINE
constexpr std::make_unsigned_t<T> as_unsigned(T val) noexcept  { return std::make_unsigned_t<T>(val); }


//! Returns r.size() as signed type (same as std::ssize in C++20)
template< typename SizedRange >  OEL_ALWAYS_INLINE
constexpr auto ssize(const SizedRange & r)
->	std::common_type_t<std::ptrdiff_t, decltype( as_signed(r.size()) )>
	{
		using S = std::common_type_t<std::ptrdiff_t, decltype( as_signed(r.size()) )>;
		return static_cast<S>(r.size());
	}
//! Returns number of elements in array as signed type
template< typename T, std::ptrdiff_t Size >  OEL_ALWAYS_INLINE
constexpr std::ptrdiff_t ssize(const T(&)[Size]) noexcept  { return Size; }


/** @brief Check if index is valid (can be used with operator[]) for array or other container-like object
*
* Negative index should give false result, however this is not ensured if the maximum value of the index type is less
* than the number of elements in r. But then you already have a problem, and should use a bigger or unsigned index type. */
template< typename Integral, typename SizedRange >
constexpr bool index_valid(const SizedRange & r, Integral index)
	{
		return as_unsigned(index) < as_unsigned(oel::ssize(r));
	}


//! Tag to select a constructor that allocates storage without filling it with objects
struct reserve_tag
{
	explicit constexpr reserve_tag() {}
};
constexpr reserve_tag reserve; //!< An instance of reserve_tag for convenience

//! Tag to specify default initialization
struct for_overwrite_t
{
	explicit constexpr for_overwrite_t() {}
};
constexpr for_overwrite_t for_overwrite;  //!< An instance of for_overwrite_t for convenience
[[deprecated]] constexpr for_overwrite_t default_init;



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


// Cannot do ADL `begin(r)` in implementation of class with begin member
template< typename Range >  OEL_ALWAYS_INLINE
constexpr auto adl_begin(Range && r) -> decltype(begin(r)) { return begin(r); }

} // namespace oel
