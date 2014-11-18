#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "debug.h"

#include <iterator>


#if !_MSC_VER || _MSC_VER >= 1900
#	define NOEXCEPT noexcept

#	define ALIGNOF alignof
#else
#	define NOEXCEPT throw()

#	define ALIGNOF __alignof
#endif


/// Obscure Efficient Template Library
namespace oetl
{

using std::size_t;

using std::begin;
using std::end;

#if _MSC_VER

using std::cbegin;
using std::cend;

#else
/**
* @brief Const version of std::begin.
* @return An iterator addressing the (const) first element in the range r. */
template<typename Range> inline
auto cbegin(const Range & r) -> decltype(begin(r))  { return begin(r); }
/**
* @brief Const version of std::end.
* @return An iterator positioned one beyond the (const) last element in the range r. */
template<typename Range> inline
auto cend(const Range & r) -> decltype(end(r))  { return end(r); }

#endif


/// Returns number of elements in r (array, container or iterator_range), signed type
template<typename Range>
auto count(const Range & r) -> typename std::iterator_traits<decltype(begin(r))>::difference_type;



/// For copy functions that return the end of both source and destination ranges
template<typename InIterator, typename OutIterator>
struct range_ends
{
	InIterator  src_end;
	OutIterator dest_end;
};


////////////////////////////////////////////////////////////////////////////////

// The rest are advanced utilities, not for users


#if __GLIBCXX__
	template<typename T>
	struct is_trivially_copyable : std::integral_constant< bool,
				__has_trivial_copy(T) && __has_trivial_assign(T) > {};
#else
	using std::is_trivially_copyable;
#endif



/// Convert iterator to pointer. This is overloaded for each contiguous memory iterator class
template<typename T>
T * to_ptr(T * ptr)  { return ptr; }

template<typename Iterator> inline
auto to_ptr(std::move_iterator<Iterator> it) NOEXCEPT
 -> decltype( to_ptr(it.base()) )  { return to_ptr(it.base()); }

#if _MSC_VER
	template<typename T, size_t S> inline
	T *       to_ptr(std::_Array_iterator<T, S> it)        { return it._Unchecked(); }
	template<typename T, size_t S> inline
	const T * to_ptr(std::_Array_const_iterator<T, S> it)  { return it._Unchecked(); }
#elif __GLIBCXX__
	template<typename T, typename U> inline
	T * to_ptr(__gnu_cxx::__normal_iterator<T *, U> it) noexcept  { return it.base(); }
#endif

namespace _detail
{
	template<typename T>    // (target, source)
	is_trivially_copyable<T> CanMemmoveArrays(T *, const T *) { return {}; }
}

/// If an InIterator range can be copied to an OutIterator range with memmove, returns std::true_type, else false_type
template<typename OutIterator, typename InIterator> inline
auto can_memmove_ranges_with(OutIterator dest, InIterator source)
 -> decltype( _detail::CanMemmoveArrays(to_ptr(dest), to_ptr(source)) )  { return {}; }

// SFINAE fallback for cases where to_ptr(iterator) is not declared or value types are not the same
inline std::false_type can_memmove_ranges_with(...)  { return {}; }

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	template<typename HasSizeRange> inline // pass dummy int to prefer this overload
	auto Count(const HasSizeRange & r, int) -> decltype(r.size()) { return r.size(); }

	template<typename Range> inline
	auto Count(const Range & r, long) -> decltype( std::distance(begin(r), end(r)) )
										  { return std::distance(begin(r), end(r)); }
}

} // namespace oetl

template<typename Range>
inline auto oetl::count(const Range & r) -> typename std::iterator_traits<decltype(begin(r))>::difference_type
{
	return _detail::Count(r, int());
}
