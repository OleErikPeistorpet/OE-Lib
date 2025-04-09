#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/contiguous_iterator_to_ptr.h" // just convenient
#include "auxi/core_util.h"
#include "auxi/range_traits.h"


/** @file
* @brief Contains make_unique_for_overwrite, as_signed/as_unsigned, index_valid, ssize and more
*/


/** @brief Take the name of a member function and wrap it in a stateless function object
*
* `wrapped(object, args)` becomes the same as `object.func(args)`.
* This macro works for function templates and overload sets, unlike std::mem_fn.
* Note that passing a function or pointer to member often optimizes worse. */
#define OEL_MEMBER_FN(func)  \
	[](auto && ob_, auto &&... args_)  \
	->	decltype( decltype(ob_)(ob_).func(decltype(args_)(args_)...) )  \
		{  return decltype(ob_)(ob_).func(decltype(args_)(args_)...); }

/** @brief Take the name of a member variable and wrap it in a stateless function object
*
* The call operator returns a forwarded reference like std::invoke. */
#define OEL_MEMBER_VAR(var)  \
	[](auto && ob_) noexcept  \
	->	decltype( (decltype(ob_)(ob_).var) )  \
		{  return (decltype(ob_)(ob_).var); }


namespace oel
{

//! Passed val of integral or enumeration type, returns val cast to the corresponding signed integer type
inline constexpr auto as_signed =
	[](auto val) noexcept -> std::make_signed_t<decltype(val)>    { return std::make_signed_t<decltype(val)>(val); };
//! Passed val of integral or enumeration type, returns val cast to the corresponding unsigned integer type
inline constexpr auto as_unsigned =
	[](auto val) noexcept -> std::make_unsigned_t<decltype(val)>  { return std::make_unsigned_t<decltype(val)>(val); };


/** @brief More generic than std::ssize, close to std::ranges::ssize
*
* Ill-formed if `r.size()` is ill-formed and `begin(r)` cannot be subtracted from `end(r)` (SFINAE-friendly) */
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


//! Returns true if index is within bounds (for `r[index]`)
/**
* Requires that `r.size()` or `end(r) - begin(r)` is valid. */
template< typename Integer, typename SizedRangeLike >
[[nodiscard]] constexpr bool index_valid(SizedRangeLike & r, Integer index)
	{
		static_assert( sizeof(Integer) >= sizeof _detail::Size(r) or std::is_unsigned_v<Integer>,
			"Mismatched index type, please use a wider integer (or unsigned)" );
		return as_unsigned(index) < as_unsigned(_detail::Size(r));
	}


//! Tag to select a constructor that allocates storage without filling it with objects
struct reserve_tag
{
	explicit reserve_tag() = default;
};
inline constexpr reserve_tag reserve; //!< An instance of reserve_tag for convenience

//! Tag to specify default initialization
struct for_overwrite_t
{
	explicit for_overwrite_t() = default;
};
inline constexpr for_overwrite_t for_overwrite; //!< An instance of for_overwrite_t for convenience

#if __cpp_lib_containers_ranges < 202202
	struct from_range_t
	{
		explicit from_range_t() = default;
	};
	inline constexpr from_range_t from_range;
#else
	using std::from_range_t;
	using std::from_range;
#endif



//! Same as `begin(range)` with a previous `using std::begin;`. For use in classes with a member named begin
inline constexpr auto adl_begin =
	[](auto && range) -> decltype(begin(range)) { return begin(range); };
//! Same as `end(range)` with a previous `using std::end;`. For use in classes with a member named end
inline constexpr auto adl_end =
	[](auto && range) -> decltype(end(range)) { return end(range); };



//! Tells whether we can call member `reallocate(pointer, size_type)` on an instance of Alloc
template< typename Alloc >
inline constexpr bool allocator_can_realloc   = _detail::CanRealloc<Alloc>(0);



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

	// MSVC needs unique names to guard against name collision due to inheritance
	template< typename FirstType_7KQw, typename EmptyType_7KQw >
	struct TightPair<FirstType_7KQw, EmptyType_7KQw, true>
	 :	EmptyType_7KQw
	{
		FirstType_7KQw first;

		TightPair() = default;
		constexpr TightPair(FirstType_7KQw f, EmptyType_7KQw s)
		 :	EmptyType_7KQw{s}, first{std::move(f)}
		{}

		OEL_ALWAYS_INLINE constexpr const EmptyType_7KQw & second() const { return *this; }
		OEL_ALWAYS_INLINE constexpr       EmptyType_7KQw & second()       { return *this; }
	};
}

} // namespace oel
