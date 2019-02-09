#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/type_traits.h"
#include "auxi/contiguous_iterator_to_ptr.h"
#include "make_unique.h"

#include <stdexcept>
#include <cstdint> // for uintmax_t


/** @file
* @brief Contains as_signed/as_unsigned, index_valid, ssize, adl_begin/adl_end, deref_args and more
*/

namespace oel
{

//! Given argument val of integral or enumeration type T, returns val cast to the signed integer type corresponding to T
template<typename T>  OEL_ALWAYS_INLINE
constexpr typename std::make_signed<T>::type
	as_signed(T val) noexcept                  { return (typename std::make_signed<T>::type) val; }
//! Given argument val of integral or enumeration type T, returns val cast to the unsigned integer type corresponding to T
template<typename T>  OEL_ALWAYS_INLINE
constexpr typename std::make_unsigned<T>::type
	as_unsigned(T val) noexcept                { return (typename std::make_unsigned<T>::type) val; }



//! Returns r.size() as signed type
template<typename SizedRange>  OEL_ALWAYS_INLINE
constexpr auto ssize(const SizedRange & r)
 -> decltype( static_cast< range_difference_t<SizedRange> >(r.size()) )
     { return static_cast< range_difference_t<SizedRange> >(r.size()); }
//! Returns number of elements in array as signed type
template<typename T, std::ptrdiff_t Size>  OEL_ALWAYS_INLINE
constexpr std::ptrdiff_t ssize(const T(&)[Size]) noexcept  { return Size; }


/** @brief Check if index is valid (can be used with operator[]) for array or other container-like object
*
* Negative index gives false result, even if the value is within range after conversion to an unsigned type,
* which happens implicitly when passed to operator[] of dynarray and std containers. */
template<typename Integral, typename SizedRange>
constexpr bool index_valid(const SizedRange & r, Integral index);



using std::begin;
using std::end;

#if __cplusplus >= 201402L or defined _MSC_VER
	using std::cbegin;   using std::cend;
	using std::crbegin;  using std::crend;
#endif

/** @brief Argument-dependent lookup non-member begin, defaults to std::begin
*
* Note the global using-directive  @code
	auto it = container.begin();     // Fails with types that don't have begin member such as built-in arrays
	auto it = std::begin(container); // Fails with types that have only non-member begin outside of namespace std
	// Argument-dependent lookup, as generic as it gets
	using std::begin; auto it = begin(container);
	auto it = adl_begin(container);  // Equivalent to line above
@endcode  */
template<typename Range>  OEL_ALWAYS_INLINE
constexpr auto adl_begin(Range & r) -> decltype(begin(r))         { return begin(r); }
//! Const version of adl_begin(), analogous to std::cbegin
template<typename Range>  OEL_ALWAYS_INLINE
constexpr auto adl_cbegin(const Range & r) -> decltype(begin(r))  { return begin(r); }

//! Argument-dependent lookup non-member end, defaults to std::end
template<typename Range>  OEL_ALWAYS_INLINE
constexpr auto adl_end(Range & r) -> decltype(end(r))         { return end(r); }
//! Const version of adl_end()
template<typename Range>  OEL_ALWAYS_INLINE
constexpr auto adl_cend(const Range & r) -> decltype(end(r))  { return end(r); }

} // namespace oel

using oel::adl_begin;
using oel::adl_cbegin;
using oel::adl_end;
using oel::adl_cend;


namespace oel
{

/** @brief Calls operator * on arguments before passing them to Func
*
* Example, sort pointers by pointed-to values, not addresses:
@code
oel::dynarray< std::unique_ptr<double> > d;
std::sort(d.begin(), d.end(), deref_args<std::less<>>{}); // std::less<double> before C++14
@endcode  */
template<typename Func>
struct deref_args
{
	Func wrapped;

	template<typename... Ts>
	auto operator()(Ts &&... args) const -> decltype( wrapped(*std::forward<Ts>(args)...) )
	                                         { return wrapped(*std::forward<Ts>(args)...); }

	using is_transparent = void;
};



//! Tag to select a constructor that allocates storage without filling it with objects
struct reserve_tag
{
	explicit constexpr reserve_tag() {}
};
constexpr reserve_tag reserve; //!< An instance of reserve_tag for convenience

//! Tag to specify default initialization
struct default_init_t
{
	explicit constexpr default_init_t() {}
};
constexpr default_init_t default_init; //!< An instance of default_init_t for convenience



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


namespace _detail
{
	struct Throw
	{	// at namespace scope this produces warnings of unreferenced function or failed inlining
		[[noreturn]] static void OutOfRange(const char * what)
		{
			OEL_THROW(std::out_of_range(what), what);
		}

		[[noreturn]] static void LengthError(const char * what)
		{
			OEL_THROW(std::length_error(what), what);
		}
	};


	template<typename Unsigned, typename Integral>
	constexpr bool IndexValid(Unsigned size, Integral i, false_type)
	{	// assumes that size never is greater than INTMAX_MAX, and INTMAX_MAX is half UINTMAX_MAX
		return static_cast<std::uintmax_t>(i) < size;
	}

	template<typename Unsigned, typename Integral>
	constexpr bool IndexValid(Unsigned size, Integral i, true_type)
	{	// found to be faster when both types are smaller than intmax_t
		return (0 <= i) & (as_unsigned(i) < size);
	}
}

} // namespace oel

template<typename Integral, typename SizedRange>
constexpr bool oel::index_valid(const SizedRange & r, Integral i)
{
	using T = decltype(oel::ssize(r));
	using NotBigInts = bool_constant<sizeof(T) < sizeof(std::uintmax_t) and sizeof i < sizeof(std::uintmax_t)>;
	return _detail::IndexValid(as_unsigned(oel::ssize(r)), i, NotBigInts{});
}