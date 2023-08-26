#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/type_traits.h"
#include "auxi/contiguous_iterator_to_ptr.h"


/** @file
* @brief Contains make_unique_for_overwrite, as_signed/as_unsigned, index_valid, ssize and more
*/

namespace oel
{

//! Passed val of integral or enumeration type, returns val cast to the corresponding signed integer type
inline constexpr auto as_signed =
	[](auto val) noexcept -> std::make_signed_t<decltype(val)>    { return std::make_signed_t<decltype(val)>(val); };
//! Passed val of integral or enumeration type, returns val cast to the corresponding unsigned integer type
inline constexpr auto as_unsigned =
	[](auto val) noexcept -> std::make_unsigned_t<decltype(val)>  { return std::make_unsigned_t<decltype(val)>(val); };


//! More generic than std::ssize, close to std::ranges::ssize
template< typename SizedRangeLike >
constexpr auto ssize(SizedRangeLike && r)
->	std::common_type_t< ptrdiff_t, decltype(as_signed( _detail::Size(r) )) >
	{
		return std::common_type_t< ptrdiff_t, decltype(as_signed( _detail::Size(r) )) >(_detail::Size(r));
	}

namespace view
{
using oel::ssize;
}


/** @brief Check if index is valid (can be used with operator[]) for sized_range (std concept) or similar
*
* Negative index should give false result, however this is not ensured if the
* maximum value of the index type is less than the number of elements in r. */
template< typename Integral, typename SizedRangeLike >
constexpr bool index_valid(SizedRangeLike & r, Integral index)
	{
		static_assert( std::is_unsigned_v<Integral> or sizeof(Integral) >= sizeof _detail::Size(r),
			"There seems to be a problem, you should use a bigger or unsigned index type" );
		return as_unsigned(index) < as_unsigned(_detail::Size(r));
	}


//! Equivalent to std::make_unique_for_overwrite (C++20), for array types with unknown bound
template< typename T,
          enable_if< _detail::isUnboundedArray<T> > = 0
>  inline
std::unique_ptr<T> make_unique_for_overwrite(size_t count)
	{
		return std::unique_ptr<T>{new std::remove_extent_t<T>[count]};
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
