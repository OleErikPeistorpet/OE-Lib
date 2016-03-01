#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "debug.h"

#include <iterator>


#if !_MSC_VER || _MSC_VER >= 1900
	#undef NOEXCEPT
	#define NOEXCEPT noexcept

	#define OEL_ALIGNOF alignof
#else
	#ifndef NOEXCEPT
		#define NOEXCEPT throw()
	#endif

	#define OEL_ALIGNOF __alignof
#endif


/// Obscure Efficient Library
namespace oel
{

using std::size_t;

using std::begin;
using std::end;

#if __cplusplus >= 201402L || _MSC_VER || __GNUC__ >= 5
	using std::cbegin;  using std::cend;
	using std::rbegin;  using std::rend;
	using std::crbegin; using std::crend;
#else
	/**
	* @brief Const version of std::begin.
	* @return An iterator addressing the (const) first element in the range r. */
	template<typename Range> inline
	constexpr auto cbegin(const Range & r) -> decltype(begin(r))  { return begin(r); }
	/**
	* @brief Const version of std::end.
	* @return An iterator positioned one beyond the (const) last element in the range r. */
	template<typename Range> inline
	constexpr auto cend(const Range & r) -> decltype(std::end(r))      { return std::end(r); }

	/**
	* @brief Like std::begin, but reverse iterator.
	* @return An iterator to the reverse-beginning of the range r. */
	template<typename Range> inline
	constexpr auto rbegin(Range & r) -> decltype(r.rbegin())        { return r.rbegin(); }
	/// Returns a const-qualified iterator to the reverse-beginning of the range r
	template<typename Range> inline
	constexpr auto crbegin(const Range & r) -> decltype(oel::rbegin(r))  { return oel::rbegin(r); }

	/**
	* @brief Like std::end, but reverse iterator.
	* @return An iterator to the reverse-end of the range r. */
	template<typename Range> inline
	constexpr auto rend(Range & r) -> decltype(r.rend())        { return r.rend(); }
	/// Returns a const-qualified iterator to the reverse-end of the range r
	template<typename Range> inline
	constexpr auto crend(const Range & r) -> decltype(oel::rend(r))  { return oel::rend(r); }
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
/// Const version of adl_begin
template<typename Range> inline
constexpr auto adl_cbegin(const Range & r) -> decltype(begin(r))  { return begin(r); }

/// Argument-dependent lookup non-member end, defaults to std::end
template<typename Range> inline
constexpr auto adl_end(Range & r) -> decltype(end(r))         { return end(r); }
/// Const version of adl_end
template<typename Range> inline
constexpr auto adl_cend(const Range & r) -> decltype(end(r))  { return end(r); }



template<typename T>
constexpr typename T::difference_type ssize_zero(const T &, int)  { return {}; }
template<typename Range>
constexpr auto ssize_zero(const Range & r, long)
 -> typename std::iterator_traits<decltype(begin(r))>::difference_type  { return {}; }

template<typename T>
using difference_type = decltype( ssize_zero(std::declval<T>(), int{}) );

/// Returns ib.size() as signed
template<typename SizedRange>
constexpr auto ssize(const SizedRange & ib) -> decltype(r.size(), difference_type<SizedRange>())  { return r.size(); }
/// Returns number of elements in array as signed
template<typename T, size_t Size>
constexpr std::ptrdiff_t ssize(const T (&)[Size]) noexcept  { return Size; }

/** @brief Returns number of elements in iterable (which begin and end can be called on), signed type
*
* Calls ssize(ib) if possible, otherwise equivalent to std::distance(begin(ib), end(ib))  */
template<typename InputRange>
difference_type<InputRange> count(const InputRange & ib);



/// Erase the elements from first to the end of container, making first the new end
template<class Container>
void erase_back(Container & ctr, typename Container::iterator first);



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
T * to_pointer_contiguous(T * ptr)  { return ptr; }

template<typename Iterator> inline
auto to_pointer_contiguous(std::move_iterator<Iterator> it) NOEXCEPT
 -> decltype( to_pointer_contiguous(it.base()) )  { return to_pointer_contiguous(it.base()); }


#if __GNUC__ == 4 && __GNUC_MINOR__ < 8 && !__clang__
	template<typename T>
	using is_trivially_destructible = std::has_trivial_destructor<T>;
#else
	using std::is_trivially_destructible;
#endif

/// Equivalent to std::is_trivially_copyable, but can be specialized for a type if you are sure memcpy is safe to copy it
template<typename T>
struct is_trivially_copyable :
	#if __GNUC__ == 4
		std::integral_constant< bool,
				__has_trivial_copy(T) && is_trivially_destructible<T>::value && std::is_copy_assignable<T>::value > {};
	#else
		std::is_trivially_copyable<T> {};
	#endif



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is implementation details


#if _MSC_VER
	template<typename T, size_t S> inline
	T *       to_pointer_contiguous(std::_Array_iterator<T, S> it)        { return it._Unchecked(); }
	template<typename T, size_t S> inline
	const T * to_pointer_contiguous(std::_Array_const_iterator<T, S> it)  { return it._Unchecked(); }

	template<typename S> inline
	typename std::_String_iterator<S>::pointer  to_pointer_contiguous(std::_String_iterator<S> it)
	{
		return it._Unchecked();
	}
	template<typename S> inline
	typename std::_String_const_iterator<S>::pointer  to_pointer_contiguous(std::_String_const_iterator<S> it)
	{
		return it._Unchecked();
	}
#elif __GLIBCXX__
	template<typename T, typename U> inline
	T * to_pointer_contiguous(__gnu_cxx::__normal_iterator<T *, U> it) noexcept  { return it.base(); }
#endif


namespace _detail
{
	template<typename T>   // (target, source)
	is_trivially_copyable<T> CanMemmoveArrays(T *, const T *) { return {}; }

	template<typename IterDest, typename IterSrc>
	auto CanMemmoveWith(IterDest dest, IterSrc src)
	 -> decltype( _detail::CanMemmoveArrays(to_pointer_contiguous(dest), to_pointer_contiguous(src)) ) { return {}; }

	// SFINAE fallback for cases where to_pointer_contiguous(iterator) would be ill-formed or return types are not compatible
	inline std::false_type CanMemmoveWith(...) { return {}; }

////////////////////////////////////////////////////////////////////////////////

	template<typename HasSizeRange> inline // pass dummy int to prefer this overload
	auto Count(const HasSizeRange & r, int) -> decltype(r.size()) { return r.size(); }

	template<typename Range> inline
	auto Count(const Range & r, long) -> decltype( std::distance(begin(r), end(r)) )
										  { return std::distance(begin(r), end(r)); }


	template<class HasEraseBack> inline
	auto EraseBack(HasEraseBack & ctr, typename HasEraseBack::iterator first, int) -> decltype(ctr.erase_back(first))
																					  { return ctr.erase_back(first); }
	template<class Container> inline
	void EraseBack(Container & ctr, typename Container::iterator first, long) { ctr.erase(first, ctr.end()); }
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


template<class Container>
inline void oel::erase_back(Container & ctr, typename Container::iterator first)
{
	_detail::EraseBack(ctr, first, int{});
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
