#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "debug.h"

#include <iterator>


#if !_MSC_VER || _MSC_VER >= 1900
	#define NOEXCEPT noexcept

	#define ALIGNOF alignof
#else
	#define NOEXCEPT throw()

	#define ALIGNOF __alignof
#endif


/// Obscure Efficient Library
namespace oel
{

using std::size_t;

using std::begin;
using std::end;

#if _MSC_VER
	using std::cbegin;
	using std::cend;
	using std::rbegin;
	using std::crbegin;
	using std::rend;
	using std::crend;
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

	/**
	* @brief Like std::begin, but reverse iterator.
	* @return An iterator to the reverse-beginning of the range r. */
	template<typename Range> inline
	auto rbegin(Range & r) -> decltype(r.rbegin())  { return r.rbegin(); }
	/// Same as crbegin(const Range &)
	template<typename Range> inline
	auto rbegin(const Range & r) -> decltype(r.rbegin())  { return r.rbegin(); }
	/// Returns a const-qualified iterator to the reverse-beginning of the range r
	template<typename Range> inline
	auto crbegin(const Range & r) -> decltype(rbegin(r))  { return rbegin(r); }

	/**
	* @brief Like std::end, but reverse iterator.
	* @return An iterator to the reverse-end of the range r. */
	template<typename Range> inline
	auto rend(Range & r) -> decltype(r.rend())  { return r.rend(); }
	/// Same as crend(const Range &)
	template<typename Range> inline
	auto rend(const Range & r) -> decltype(r.rend())  { return r.rend(); }
	/// Returns a const-qualified iterator to the reverse-end of the range r
	template<typename Range> inline
	auto crend(const Range & r) -> decltype(rend(r))  { return rend(r); }
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



/// If an IteratorSource range can be copied to an IteratorDest range with memmove, is-a std::true_type, else false_type
template<typename IteratorDest, typename IteratorSource>
struct can_memmove_with;


/// Convert iterator to pointer. This should be overloaded for each contiguous memory iterator class
template<typename T> inline
T * to_ptr(T * ptr)  { return ptr; }

template<typename Iterator> inline
auto to_ptr(std::move_iterator<Iterator> it) NOEXCEPT
 -> decltype( to_ptr(it.base()) )  { return to_ptr(it.base()); }


#if __GLIBCXX__
	template<typename T>
	using is_trivially_copyable = std::integral_constant< bool,
									__has_trivial_copy(T) && __has_trivial_assign(T) >;
#else
	using std::is_trivially_copyable;
#endif

#if __GNUC__ == 4 && __GNUC_MINOR__ < 8 && !__clang__
	template<typename T>
	using is_trivially_destructible = std::has_trivial_destructor<T>;
#else
	using std::is_trivially_destructible;
#endif



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is implementation details


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
	template<typename T>   // (target, source)
	is_trivially_copyable<T> CanMemmoveArrays(T *, const T *) { return {}; }

	template<typename IterDest, typename IterSrc>
	auto CanMemmoveWith(IterDest dest, IterSrc src)
	 -> decltype( CanMemmoveArrays(to_ptr(dest), to_ptr(src)) ) { return {}; }

	// SFINAE fallback for cases where to_ptr(iterator) is not declared or value types are not the same
	inline std::false_type CanMemmoveWith(...) { return {}; }

////////////////////////////////////////////////////////////////////////////////

	template<typename HasSizeRange> inline // pass dummy int to prefer this overload
	auto Count(const HasSizeRange & r, int) -> decltype(r.size()) { return r.size(); }

	template<typename Range> inline
	auto Count(const Range & r, long) -> decltype( std::distance(begin(r), end(r)) )
										  { return std::distance(begin(r), end(r)); }
}

} // namespace oel

/// @cond FALSE
template<typename IteratorDest, typename IteratorSource>
struct oel::can_memmove_with : decltype( _detail::CanMemmoveWith(std::declval<IteratorDest>(),
																 std::declval<IteratorSource>()) ) {};
/// @endcond


template<typename Range>
inline auto oel::count(const Range & r) -> typename std::iterator_traits<decltype(begin(r))>::difference_type
{
	return _detail::Count(r, int{});
}

////////////////////////////////////////////////////////////////////////////////

#if __GNUC__
	#define OEL_PUSH_IGNORE_UNUSED_VALUE  \
		_Pragma("GCC diagnostic push")  \
		_Pragma("GCC diagnostic ignored \"-Wunused-value\"")
	#define OEL_POP_DIAGNOSTIC _Pragma("GCC diagnostic pop")
#else
	#define OEL_PUSH_IGNORE_UNUSED_VALUE
	#define OEL_POP_DIAGNOSTIC
#endif
