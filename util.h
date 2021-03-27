#pragma once

// Copyright 2015 Ole Erik Peistorpet
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


//! More generic than std::ssize, close to std::ranges::ssize
template< typename SizedRangeLike >
constexpr auto ssize(SizedRangeLike && r)
->	std::common_type_t< ptrdiff_t, decltype(as_signed( _detail::Size(r) )) >
	{
		return std::common_type_t< ptrdiff_t, decltype(as_signed( _detail::Size(r) )) >(_detail::Size(r));
	}


/** @brief Check if index is valid (can be used with operator[]) for sized_range (std concept) or similar
*
* Negative index should give false result, however this is not ensured if the
* maximum value of the index type is less than the number of elements in r. */
template< typename Integral, typename SizedRangeLike >
constexpr bool index_valid(SizedRangeLike & r, Integral index)
	{
		static_assert( std::is_unsigned<Integral>::value or sizeof(Integral) >= sizeof _detail::Size(r),
			"There seems to be a problem, you should use a bigger or unsigned index type" );
		return as_unsigned(index) < as_unsigned(_detail::Size(r));
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



template< typename T >
struct
#ifdef __GNUC__
	__attribute__((may_alias))
#endif
	storage_for
{
	alignas(T) unsigned char as_bytes[sizeof(T)];
};


namespace _detail
{
	template< typename T, typename U,
	          bool = std::is_empty<U>::value >
	struct TightPair
	{
		T first;
		U _sec;

		OEL_ALWAYS_INLINE constexpr const U & second() const { return _sec; }
		OEL_ALWAYS_INLINE constexpr       U & second()       { return _sec; }
	};

	template< typename Type_unique_name_for_MSVC, typename Empty_type_MSVC_unique_name >
	struct TightPair<Type_unique_name_for_MSVC, Empty_type_MSVC_unique_name, true>
	 :	Empty_type_MSVC_unique_name
	{
		Type_unique_name_for_MSVC first;

		TightPair() = default;
		constexpr TightPair(Type_unique_name_for_MSVC f, Empty_type_MSVC_unique_name s)
		 :	Empty_type_MSVC_unique_name{s}, first{std::move(f)}
		{}

		OEL_ALWAYS_INLINE constexpr const Empty_type_MSVC_unique_name & second() const { return *this; }
		OEL_ALWAYS_INLINE constexpr       Empty_type_MSVC_unique_name & second()       { return *this; }
	};
}

} // namespace oel
