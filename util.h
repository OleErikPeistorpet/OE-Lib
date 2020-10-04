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


#if __cpp_lib_ssize
	using std::ssize;
#else
	//! Same as std::ssize (C++20)
	template< typename SizedRange >  OEL_ALWAYS_INLINE
	constexpr auto ssize(const SizedRange & r)
	->	std::common_type_t<ptrdiff_t, decltype( as_signed(r.size()) )>
		{
			using S = std::common_type_t<ptrdiff_t, decltype( as_signed(r.size()) )>;
			return static_cast<S>(r.size());
		}
	//! Equivalent to array overload of std::ssize
	template< typename T, ptrdiff_t Size >  OEL_ALWAYS_INLINE
	constexpr ptrdiff_t ssize(const T(&)[Size]) noexcept  { return Size; }
#endif


/** @brief Check if index is valid (can be used with operator[]) for array or other container-like object
*
* Negative index should give false result. However, this is not ensured if the index type holds more
* bits than `int`, yet the maximum value of index type is less than the number of elements in r.
* This is not a concern in practice. */
template< typename Integral, typename SizedRange >
constexpr bool index_valid(const SizedRange & r, Integral index);



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


namespace _detail
{
	template< typename T, typename SansCVRef >
	struct ShouldPassByValue : bool_constant<
		#if defined _MSC_VER && (_M_IX86 || _M_AMD64)
			sizeof(SansCVRef) <= 2 * sizeof(int) and
			(	std::is_trivially_copy_constructible<SansCVRef>::value or
				(std::is_move_constructible<SansCVRef>::value and
				 !std::is_lvalue_reference<T>::value and !std::is_const<T>::value) )
		#else
			sizeof(SansCVRef) <= 2 * sizeof(void *) and
				std::is_trivially_copy_constructible<SansCVRef>::value and std::is_trivially_destructible<SansCVRef>::value
		#endif
		> {};

	template< typename T,
		typename SansCVRef = std::remove_cv_t< std::remove_reference_t<T> >
	>
	auto ForwardT(T &&)
	->	std::conditional_t<
			conjunctionV<
				bool_constant< !std::is_function<SansCVRef>::value >,
				ShouldPassByValue<T, SansCVRef>
			>,
			SansCVRef,
			T &&
		>;


	template< typename RandomAccessIter >
	constexpr auto SentinelAt(RandomAccessIter it, iter_difference_t<RandomAccessIter> n)
	->	decltype( std::true_type{iter_is_random_access<RandomAccessIter>()},
		          it + n )
		 { return it + n; }



	using BigUint =
	#if ULONG_MAX > UINT_MAX
		unsigned long;
	#else
		unsigned long long;
	#endif

	template< typename Unsigned >
	constexpr bool IndexValid(Unsigned size, BigUint i, false_type)
	{
		return i < size;
	}

	template< typename Unsigned, typename Integral >
	constexpr bool IndexValid(Unsigned size, Integral i, true_type)
	{	// casting to uint64_t when both types are smaller and using just 'i < size' was found to be slower
		return (0 <= i) & (as_unsigned(i) < size);
	}
}

} // namespace oel

template< typename Integral, typename SizedRange >
constexpr bool oel::index_valid(const SizedRange & r, Integral index)
{
	using T = decltype(oel::ssize(r));
	using NeitherIsBig = bool_constant<sizeof(T) < sizeof(_detail::BigUint) and sizeof index < sizeof(_detail::BigUint)>;
	return _detail::IndexValid(as_unsigned(oel::ssize(r)), index, NeitherIsBig{});
}