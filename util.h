#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "user_traits.h"
#include "error_handling.h"

#ifndef OEL_NO_BOOST
	#include <boost/iterator/iterator_categories.hpp>
#endif
#include <iterator>
#include <cstdint>
#include <string.h> // for memcpy


/** @file
* @brief Utilities, included throughout the library
*
* Contains as_signed/as_unsigned, index_valid, ssize, adl_begin, adl_end, deref_args and more.
*/

//! Functions marked with OEL_NOEXCEPT_NDEBUG will only throw exceptions from OEL_ALWAYS_ASSERT (none by default)
#if defined(NDEBUG) && OEL_MEM_BOUND_DEBUG_LVL == 0
	#define OEL_NOEXCEPT_NDEBUG noexcept
#else
	#define OEL_NOEXCEPT_NDEBUG
#endif


namespace oel
{

//! Same as std::enable_if_t<Condition, int>. Type int is intended as unused dummy
template<bool Condition>
using enable_if = typename std::enable_if<Condition, int>::type;



//! Given argument val of integral or enumeration type T, returns val cast to the signed integer type corresponding to T
template<typename T> inline
constexpr typename std::make_signed<T>::type   as_signed(T val) noexcept  { return (typename std::make_signed<T>::type)val; }
//! Given argument val of integral or enumeration type T, returns val cast to the unsigned integer type corresponding to T
template<typename T> inline
constexpr typename std::make_unsigned<T>::type as_unsigned(T val) noexcept
	{
		return (typename std::make_unsigned<T>::type)val;
	}



//! Returns r.size() as signed (SizedRange::difference_type)
template<typename SizedRange> inline
constexpr auto ssize(const SizedRange & r)
 -> decltype( static_cast<typename SizedRange::difference_type>(r.size()) )
     { return static_cast<typename SizedRange::difference_type>(r.size()); }
//! Returns number of elements in array as signed type
template<typename T, std::ptrdiff_t Size> inline
constexpr std::ptrdiff_t ssize(const T(&)[Size]) noexcept  { return Size; }


///@{
//! Check if index is valid (can be used with operator[]) for array or other range.
template< typename UnsignedInt, typename SizedRange,
          enable_if<std::is_unsigned<UnsignedInt>::value> = 0 > inline
constexpr bool index_valid(const SizedRange & r, UnsignedInt index)   { return index < as_unsigned(oel::ssize(r)); }

template<typename SizedRange> inline
constexpr bool index_valid(const SizedRange & r, std::int32_t index)  { return 0 <= index && index < oel::ssize(r); }

template<typename SizedRange> inline
constexpr bool index_valid(const SizedRange & r, std::int64_t index)
	{	// assumes that r.size() is never greater than INT64_MAX
		return static_cast<std::uint64_t>(index) < as_unsigned(oel::ssize(r));
	}
///@}


using std::begin;
using std::end;

#if __cplusplus >= 201402L || _MSC_VER
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
template<typename Range> inline
constexpr auto adl_begin(Range & r) -> decltype(begin(r))         { return begin(r); }
//! Const version of adl_begin(), analogous to std::cbegin
template<typename Range> inline
constexpr auto adl_cbegin(const Range & r) -> decltype(begin(r))  { return begin(r); }

//! Argument-dependent lookup non-member end, defaults to std::end
template<typename Range> inline
constexpr auto adl_end(Range & r) -> decltype(end(r))         { return end(r); }
//! Const version of adl_end()
template<typename Range> inline
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
class deref_args
{
	Func _f;

public:
	deref_args(Func f = Func{})  : _f(std::move(f)) {}

	template<typename... Ts>
	auto operator()(Ts &&... args) const -> decltype( _f(*std::forward<Ts>(args)...) )
	                                         { return _f(*std::forward<Ts>(args)...); }

	using is_transparent = void;
};



//! Convert iterator to pointer. This should be overloaded for each class of contiguous iterator (C++17 concept)
template<typename T> inline
T * to_pointer_contiguous(T * ptr) noexcept  { return ptr; }

template<typename Iterator> inline
auto to_pointer_contiguous(std::move_iterator<Iterator> it) noexcept
 -> decltype( to_pointer_contiguous(it.base()) )  { return to_pointer_contiguous(it.base()); }


//! If an IteratorSource range can be copied to an IteratorDest range with memmove, is-a true_type, else false_type
template<typename IteratorDest, typename IteratorSource>
struct can_memmove_with;



//! Tag to select a constructor that allocates storage without filling it with objects
struct reserve_tag
{	explicit reserve_tag() {}
}
const reserve; //!< An instance of reserve_tag to pass

//! Tag to specify default initialization
struct default_init_tag
{	explicit default_init_tag() {}
}
const default_init; //!< An instance of default_init_tag to pass



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


using std::size_t;


template<typename Iterator>
using iterator_difference_t = typename std::iterator_traits<Iterator>::difference_type;

template<typename Iterator>
using iterator_traversal_t
#ifndef OEL_NO_BOOST
	= typename boost::iterator_traversal<Iterator>::type;

	using boost::single_pass_traversal_tag;
	using boost::forward_traversal_tag;
	using boost::random_access_traversal_tag;
#else
	= typename std::iterator_traits<Iterator>::iterator_category;

	using single_pass_traversal_tag = std::input_iterator_tag;
	using forward_traversal_tag = std::forward_iterator_tag;
	using random_access_traversal_tag = std::random_access_iterator_tag;
#endif


#if __GLIBCXX__
	template<typename T, typename C> inline
	T * to_pointer_contiguous(__gnu_cxx::__normal_iterator<T *, C> it) noexcept
	{
		return it.base();
	}
	template<typename Ptr, typename C> inline
	typename std::pointer_traits<Ptr>::element_type *
		to_pointer_contiguous(__gnu_cxx::__normal_iterator<Ptr, C> it) noexcept
	{
		return it.base().operator->();
	}
#elif _LIBCPP_VERSION
	template<typename T> inline
	T * to_pointer_contiguous(std::__wrap_iter<T *> it) noexcept { return __unwrap_iter(it); }
#elif _MSC_VER
	template<typename T, size_t S> inline
	T *       to_pointer_contiguous(std::_Array_iterator<T, S> it) noexcept       { return _Unchecked(it); }

	template<typename T, size_t S> inline
	const T * to_pointer_contiguous(std::_Array_const_iterator<T, S> it) noexcept { return _Unchecked(it); }

	template<typename S> inline
	typename std::_String_iterator<S>::pointer  to_pointer_contiguous(std::_String_iterator<S> it) noexcept
	{
		return _Unchecked(it);
	}
	template<typename S> inline
	typename std::_String_const_iterator<S>::pointer  to_pointer_contiguous(std::_String_const_iterator<S> it) noexcept
	{
		return _Unchecked(it);
	}
#endif


namespace _detail
{
	inline void MemcpyMaybeNull(void * dest, const void * src, size_t nBytes)
	{	// memcpy(nullptr, nullptr, 0) is undefined behavior.
		// The problem is that checking can have significant performance hit
	#if OEL_GCC_VERSION >= 409
		if (nBytes > 0)
	#endif
			::memcpy(dest, src, nBytes);
	}


	template<typename T>   // (target, source)
	is_trivially_copyable<T> CanMemmoveArrays(T *, const T *);

	template<typename IterDest, typename IterSrc>
	auto CanMemmoveWith(IterDest dest, IterSrc src)
	 -> decltype( _detail::CanMemmoveArrays(to_pointer_contiguous(dest), to_pointer_contiguous(src)) );

	// SFINAE fallback for cases where to_pointer_contiguous(iterator) would be ill-formed or return types are not compatible
	false_type CanMemmoveWith(...);
}

} // namespace oel

//! @cond FALSE
template<typename IteratorDest, typename IteratorSource>
struct oel::can_memmove_with : decltype( _detail::CanMemmoveWith(std::declval<IteratorDest>(),
																 std::declval<IteratorSource>()) ) {};
//! @endcond

template<typename T>
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};
