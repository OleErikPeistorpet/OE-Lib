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


/** @brief More generic than std::ssize, close to std::ranges::ssize
*
* Ill-formed if `r.size()` is ill-formed and `begin(r)` cannot be subtracted from `end(r)` (SFINAE-friendly) */
template< typename SizedRangeLike >
constexpr auto ssize(SizedRangeLike && r)
->	std::common_type_t< ptrdiff_t, decltype(as_signed( _detail::Size(r) )) >
	{
		return std::common_type_t< ptrdiff_t, decltype(as_signed( _detail::Size(r) )) >(_detail::Size(r));
	}


/** @brief Check if index is valid (within bounds for operator[])
*
* Requires that `r.size()` or `end(r) - begin(r)` is valid. */
template< typename Integral, typename SizedRangeLike >
constexpr bool index_valid(SizedRangeLike & r, Integral index)
	{
		static_assert( sizeof(Integral) >= sizeof _detail::Size(r) or std::is_unsigned_v<Integral>,
			"Mismatched index type, please use a wider integer (or unsigned)" );
		return as_unsigned(index) < as_unsigned(_detail::Size(r));
	}


//! Tag to select a constructor that allocates storage without filling it with objects
struct reserve_tag
{
	explicit constexpr reserve_tag() {}
};
inline constexpr reserve_tag reserve; //!< An instance of reserve_tag for convenience

//! Tag to specify default initialization
struct for_overwrite_t
{
	explicit constexpr for_overwrite_t() {}
};
inline constexpr for_overwrite_t for_overwrite; //!< An instance of for_overwrite_t for convenience



//! Same as `begin(range)` with a previous `using std::begin;`. For use in classes with a member named begin
inline constexpr auto adl_begin =
	[](auto && range) -> decltype(begin(range)) { return begin(range); };
//! Same as `end(range)` with a previous `using std::end;`. For use in classes with a member named end
inline constexpr auto adl_end =
	[](auto && range) -> decltype(end(range)) { return end(range); };



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


template< typename T >
struct
#ifdef __GNUC__
	[[gnu::may_alias]]
#endif
	storage_for
{
	alignas(T) unsigned char as_bytes[sizeof(T)];
};


namespace _detail
{
	template< typename T, typename U,
	          bool = std::is_empty_v<U> >
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
