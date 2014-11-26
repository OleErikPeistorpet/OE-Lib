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
* @return An iterator addressing the (const) first element in iterable. */
template<typename Iterable> inline
auto cbegin(const Iterable & ib) -> decltype(begin(ib))  { return begin(ib); }
/**
* @brief Const version of std::end.
* @return An iterator positioned one beyond the (const) last element in iterable. */
template<typename Iterable> inline
auto cend(const Iterable & ib) -> decltype(end(ib))  { return end(ib); }

#endif


/// Returns number of elements in iterable (which begin and end can be called on), signed type
template<typename Iterable>
auto count(const Iterable & ib) -> typename std::iterator_traits<decltype(begin(ib))>::difference_type;



/// Create a std::move_iterator from InputIterator
template<typename InputIterator> inline
std::move_iterator<InputIterator> make_move_iter(InputIterator it)  { return std::make_move_iterator(it); }


/// For copy functions that return the end of both source and destination Iterables
template<typename InIterator, typename OutIterator>
struct end_iterators
{
	InIterator  src_end;
	OutIterator dest_end;
};


////////////////////////////////////////////////////////////////////////////////

// The rest are advanced utilities, not for users


template<bool Value>
using bool_constant = std::integral_constant<bool, Value>;



#if __GLIBCXX__
	template<typename T>
	struct is_trivially_copyable : bool_constant<__has_trivial_copy(T) && __has_trivial_assign(T)> {};
#else
	using std::is_trivially_copyable;
#endif



/// Convert iterator to pointer. This is overloaded for each contiguous memory iterator class
template<typename T> inline
T * to_ptr(T * ptr)  { return ptr; }

template<typename Iterator> inline
auto to_ptr(std::move_iterator<Iterator> it) NOEXCEPT
 -> decltype( to_ptr(it.base()) )  { return to_ptr(it.base()); }

#if _MSC_VER
	template<typename T, size_t S> inline
	T *       to_ptr(std::_Array_iterator<T, S> it)        { return it._Unchecked(); }
	template<typename T, size_t S> inline
	const T * to_ptr(std::_Array_const_iterator<T, S> it)  { return it._Unchecked(); }

	template<typename S> inline
	typename std::_String_iterator<S>::pointer  to_ptr(std::_String_iterator<S> it)
	{
		return it._Unchecked();
	}
	template<typename S> inline
	typename std::_String_const_iterator<S>::pointer  to_ptr(std::_String_const_iterator<S> it)
	{
		return it._Unchecked();
	}
#elif __GLIBCXX__
	template<typename T, typename U> inline
	T * to_ptr(__gnu_cxx::__normal_iterator<T *, U> it) noexcept  { return it.base(); }
#endif

namespace _detail
{
	template<typename T> inline   // (target, source)
	is_trivially_copyable<T> CanMemmoveArrays(T *, const T *) { return {}; }
}

#if _MSC_VER
#	pragma warning(push)
#	pragma warning(disable: 4100)
#endif
/// If an InIterator range can be copied to an OutIterator range with memmove, returns std::true_type, else false_type
template<typename IteratorDest, typename IteratorSrc> inline
auto can_memmove_ranges_with(IteratorDest dest, IteratorSrc source)
 -> decltype( _detail::CanMemmoveArrays(to_ptr(dest), to_ptr(source)) )  { return {}; }

#if _MSC_VER
#	pragma warning(pop)
#endif

// SFINAE fallback for cases where to_ptr(iterator) is not declared or value types are not the same
inline std::false_type can_memmove_ranges_with(...)  { return {}; }

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	template<typename HasSizeItbl> inline // pass dummy int to prefer this overload
	auto Count(const HasSizeItbl & ib, int) -> decltype(ib.size()) { return ib.size(); }

	template<typename Iterable> inline
	auto Count(const Iterable & ib, long) -> decltype( std::distance(begin(ib), end(ib)) )
											  { return std::distance(begin(ib), end(ib)); }
}

} // namespace oetl

template<typename Iterable>
inline auto oetl::count(const Iterable & ib) -> typename std::iterator_traits<decltype(begin(ib))>::difference_type
{
	return _detail::Count(ib, int{});
}
