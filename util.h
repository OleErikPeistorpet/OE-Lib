#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/contiguous_iterator_to_ptr.h" // just convenient
#include "auxi/core_util.h"
#include "auxi/range_traits.h"


/** @file
* @brief Contains as_signed/as_unsigned, ssize, index_valid and more
*/


//! Take the name of a member function and wrap it in a stateless lambda
/**
* Calling `wrapped(object, args)` becomes the same as `object.func(args)`.
* This macro works for function templates and overload sets, unlike std::mem_fn.
*
* Note that passing around a function object (which this is) often
* optimizes better than a function (as pointer) or pointer to member.
*/
#define OEL_MEMBER_FN(func)  \
	[](auto && ob_, auto &&... args_)  \
	->	decltype( decltype(ob_)(ob_).func( decltype(args_)(args_)... ) )  \
		{  return decltype(ob_)(ob_).func( decltype(args_)(args_)... ); }

//! Take the name of a member variable and wrap it in a stateless lambda
/**
* The call operator returns a forwarded reference, like std::invoke.
*/
#define OEL_MEMBER_VAR(var)  \
	[](auto && ob_) noexcept  \
	->	decltype( ( decltype(ob_)(ob_).var ) )  \
		{  return ( decltype(ob_)(ob_).var ); }


namespace oel
{

//! Passed val of integral or enumeration type, returns val cast to the corresponding signed integer type
inline constexpr auto as_signed =
	[](auto val) noexcept -> std::make_signed_t< decltype(val) >
	{
		return std::make_signed_t< decltype(val) >(val);
	};
//! Passed val of integral or enumeration type, returns val cast to the corresponding unsigned integer type
inline constexpr auto as_unsigned =
	[](auto val) noexcept -> std::make_unsigned_t< decltype(val) >
	{
		return std::make_unsigned_t< decltype(val) >(val);
	};


//! Same as std::ranges::ssize, except it can be found by ADL
template< typename SizedRangeLike >
constexpr auto ssize(SizedRangeLike && r)
	noexcept(noexcept( ranges::ssize(static_cast<SizedRangeLike &&>(r)) ))
->	decltype(          ranges::ssize(static_cast<SizedRangeLike &&>(r)) )
	{        return    ranges::ssize(static_cast<SizedRangeLike &&>(r)); }


//! Returns true if index is within bounds (for `r[index]`)
/**
* Requires that `r.size()` or `end(r) - begin(r)` is valid. */
template< typename Integer, typename SizedRangeLike >
[[nodiscard]] constexpr bool index_valid(SizedRangeLike && r, Integer index)
	{
		static_assert( sizeof(Integer) >= sizeof ranges::size(r) or std::is_unsigned_v<Integer>,
			"Mismatched index type, please use a wider integer (or unsigned)" );
		return as_unsigned(index) < as_unsigned(ranges::size(r));
	}


namespace view
{
using oel::ssize;
using oel::index_valid;
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



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


//! Overloads unwrap std::counted_iterator
template< typename Iterator >
Iterator iter_uncounted(Iterator it)  { return it; }

template< typename Iterator >
Iterator iter_uncounted(std::counted_iterator<Iterator> it)  { return std::move(it).base(); }

template< typename Iterator >
auto     iter_uncounted(std::move_iterator< std::counted_iterator<Iterator> > it)
	{
		return std::move_iterator{std::move(it).base().base()};
	}

//! Like std::ranges::borrowed_iterator_t, but strips away std::counted_iterator
template< typename Range >
using _uncountedBorrowedIteratorT =
	std::conditional_t<
		std::is_lvalue_reference_v<Range> or ranges::enable_borrowed_range< std::remove_cvref_t<Range> >,
		decltype( iter_uncounted( ranges::begin(std::declval<Range &>()) ) ),
		ranges::dangling
	>;


//! Tells whether we can call member `reallocate(pointer, size_type)` on an instance of Alloc
template< typename Alloc >
constexpr auto allocator_can_realloc()
->	decltype( Alloc::can_reallocate() )
	{  return Alloc::can_reallocate(); }

template< typename, typename... None >
constexpr bool allocator_can_realloc(None...)  { return false; }


namespace _detail
{
	template< typename T >
	struct
	#ifdef __GNUC__
		[[gnu::may_alias]]
	#endif
		RelocateWrap
	{
		alignas(T) unsigned char bytes[sizeof(T)];
	};
}

} // namespace oel
