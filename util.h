#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "range_view.h"

#include <algorithm>
#include <memory>
#include <cstdint>
#include <string.h>


/** @file
* @brief Utilities, including efficient range algorithms
*
* Designed to interface with the standard library. Contains erase functions, copy functions, make_unique and more.
*/

namespace oel
{

/// Given argument val of integral or enumeration type T, returns val cast to the signed integer type corresponding to T
template<typename T> inline
constexpr typename std::make_signed<T>::type   as_signed(T val) noexcept  { return (typename std::make_signed<T>::type)val; }
/// Given argument val of integral or enumeration type T, returns val cast to the unsigned integer type corresponding to T
template<typename T> inline
constexpr typename std::make_unsigned<T>::type as_unsigned(T val) noexcept
	{ return (typename std::make_unsigned<T>::type)val; }



/** @brief Erase the element at index from container without maintaining order of elements.
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template<typename RandomAccessContainer> inline
void erase_unordered(RandomAccessContainer & c, typename RandomAccessContainer::size_type index)
{
	c[index] = std::move(c.back());
	c.pop_back();
}

/**
* @brief Erase from container all elements for which predicate returns true
*
* This function mimics feature from paper N4009 (C++17). Wraps std::remove_if  */
template<typename Container, typename UnaryPredicate>
void erase_if(Container & c, UnaryPredicate pred);
/**
* @brief Erase consecutive duplicate elements in container. Wraps std::unique
*
* To erase duplicates anywhere, sort container contents first. (Or just use std::set or unordered_set)  */
template<typename Container>
void erase_successive_dup(Container & c);



template<typename IteratorSource, typename IteratorDest>
struct last_iterators
{
	IteratorSource src_last;
	IteratorDest   dest_last;
};

/**
* @brief Copies the elements in source into the range beginning at dest
* @return struct containing begin(source) and dest, both incremented by number of elements in source
*
* If the ranges overlap, behavior is undefined (uses memcpy when possible).
* To move instead of copy, pass view::move(source). To mimic std::copy_n, use view::counted  */
template<typename InputRange, typename OutputIterator>
auto copy_unsafe(const InputRange & source, OutputIterator dest) -> last_iterators<decltype(begin(source)), OutputIterator>;

/**
* @brief Copies the elements in the range source into dest, throws std::out_of_range if dest is smaller than source
* @return iterator begin(dest) incremented by number of elements in source
*
* The ranges shall not overlap, except if begin(source) equals begin(dest) (self assign).
* To move instead of copy, pass view::move(source)  */
template<typename SizedInputRange, typename SizedOutputRange>
auto copy(const SizedInputRange & source, SizedOutputRange & dest) -> decltype(begin(dest));
/**
* @brief Copies as many elements as will fit in dest
* @return number of elements copied, lesser of source size and dest size
*
* Otherwise same as copy(const SizedInputRange &, SizedOutputRange &)  */
template<typename SizedInputRange, typename SizedOutputRange>
std::ptrdiff_t copy_fit(const SizedInputRange & source, SizedOutputRange & dest);


///@{
/// Check if index is valid (can be used with operator[]) for array or other range.
template< typename UnsignedInt, typename SizedRange,
		  typename = enable_if_t<std::is_unsigned<UnsignedInt>::value> > inline
bool index_valid(const SizedRange & r, UnsignedInt index)   { return index < oel::as_unsigned(oel::ssize(r)); }

template<typename SizedRange> inline
bool index_valid(const SizedRange & r, std::int32_t index)  { return 0 <= index && index < oel::ssize(r); }

template<typename SizedRange> inline
bool index_valid(const SizedRange & r, std::int64_t index)
{	// assumes that r.size() is never greater than INT64_MAX
	return static_cast<std::uint64_t>(index) < oel::as_unsigned(oel::ssize(r));
}
///@}


/**
* @brief Same as std::make_unique, but performs direct-list-initialization if there is no matching constructor
*
* (Works for aggregates.) http://open-std.org/JTC1/SC22/WG21/docs/papers/2015/n4462.html  */
template< typename T, typename... Args, typename = enable_if_t<!std::is_array<T>::value> >
std::unique_ptr<T> make_unique(Args &&... args);

/// Equivalent to std::make_unique (array version).
template< typename T, typename = enable_if_t<std::is_array<T>::value> >
std::unique_ptr<T> make_unique(size_t arraySize);
/**
* @brief Array is default-initialized, can be significantly faster for non-class elements
*
* Non-class elements get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
template< typename T, typename = enable_if_t<std::is_array<T>::value> >
std::unique_ptr<T> make_unique_default(size_t arraySize);


///@{
/// For generic code that may use either dynarray or std library container
template<typename Container, typename InputRange> inline
void assign(Container & dest, const InputRange & source)  { dest.assign(begin(source), end(source)); }

template<typename Container, typename InputRange> inline
void append(Container & dest, const InputRange & source)  { dest.insert(dest.end(), begin(source), end(source)); }

template<typename Container, typename InputRange> inline
typename Container::iterator insert(Container & dest, typename Container::const_iterator pos, const InputRange & source)
													{ return dest.insert(pos, begin(source), end(source)); }
///@}


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



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


/// @cond INTERNAL
namespace _detail
{
	template<typename Container> inline
	void EraseEnd(Container & c, typename Container::iterator first) { c.erase(first, c.end()); }

	template<typename Container, typename UnaryPred> inline
	auto RemoveIf(Container & c, UnaryPred p, int) -> decltype(c.remove_if(p)) { return c.remove_if(p); }

	template<typename Container, typename UnaryPred>
	void RemoveIf(Container & c, UnaryPred p, long)
	{
		_detail::EraseEnd( c, std::remove_if(begin(c), end(c), p) );
	}

	template<typename Container> inline // pass dummy int to prefer this overload
	auto Unique(Container & c, int) -> decltype(c.unique()) { return c.unique(); }

	template<typename Container>
	void Unique(Container & c, long)
	{
		_detail::EraseEnd( c, std::unique(begin(c), end(c)) );
	}

////////////////////////////////////////////////////////////////////////////////

	template<typename InputIter, typename InputRange, typename OutputIter> inline
	auto Copy(const InputRange & src, OutputIter dest, false_type, int)
	 -> decltype(end(src), last_iterators<InputIter, OutputIter>{})
	{	// end(src) valid
		auto f = begin(src); auto l = end(src);
		while (f != l)
		{
			*dest = *f;
			++dest; ++f;
		}
		return {f, dest};
	}

	template<typename InputIter, typename InputRange, typename OutputIter> inline
	last_iterators<InputIter, OutputIter> Copy(const InputRange & src, OutputIter dest, false_type, long)
	{	// fallback for no src end
		auto f = begin(src);
		for (auto n = src.size(); 0 < n; --n)
		{
			*dest = *f;
			++dest; ++f;
		}
		return {f, dest};
	}

	template<typename CntigusIter, typename CntigusRange, typename CntigusIter2> inline
	last_iterators<CntigusIter, CntigusIter2> Copy(const CntigusRange & src, CntigusIter2 const dest, true_type, int)
	{	// can use memcpy
		auto const first = begin(src);
		auto const n = oel::ssize(src);
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (0 != n)
		{	// Dereference iterators at bounds, this detects out of range errors if they are checked iterators
			(void)*first; (void)*dest;
			(void)*(dest + (n - 1));
		}
	#endif
		::memcpy(to_pointer_contiguous(dest), to_pointer_contiguous(first), sizeof(*first) * n);
		return {end(src), dest + n};
	}
}
/// @endcond

} // namespace oel

template<typename InputRange, typename OutputIterator>
inline auto oel::copy_unsafe(const InputRange & src, OutputIterator dest)
 -> last_iterators<decltype(begin(src)), OutputIterator>
{
	using InIter = decltype(begin(src));
	return _detail::Copy<InIter>(src, dest, can_memmove_with<OutputIterator, InIter>(), 0);
}

template<typename SizedInputRange, typename SizedOutputRange>
auto oel::copy(const SizedInputRange & src, SizedOutputRange & dest) -> decltype(begin(dest))
{
	if (oel::ssize(src) <= oel::ssize(dest))
		return oel::copy_unsafe(src, begin(dest)).dest_last;
	else
		throw std::out_of_range("Too small dest for oel::copy");
}

template<typename SizedInputRange, typename SizedOutputRange>
std::ptrdiff_t oel::copy_fit(const SizedInputRange & src, SizedOutputRange & dest)
{
	auto const n = (std::min)(oel::ssize(src), oel::ssize(dest));
	oel::copy_unsafe(view::counted(begin(src), n), begin(dest));
	return n;
}


template<typename Container, typename UnaryPredicate>
inline void oel::erase_if(Container & c, UnaryPredicate p)
{
	_detail::RemoveIf(c, p, int{});
}

template<typename Container>
inline void oel::erase_successive_dup(Container & c)
{
	_detail::Unique(c, int{});
}

////////////////////////////////////////////////////////////////////////////////

namespace oel
{
namespace _detail
{
	template<typename T, typename... Args> inline
	T * New(std::true_type, Args &&... args)
	{
		return new T(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args> inline
	T * New(std::false_type, Args &&... args)
	{
		return new T{std::forward<Args>(args)...};
	}
}
}

template<typename T, typename... Args, typename>
inline std::unique_ptr<T>  oel::make_unique(Args &&... args)
{
	T * p = _detail::New<T>(std::is_constructible<T, Args...>(), std::forward<Args>(args)...);
	return std::unique_ptr<T>(p);
}

#define OEL_MAKE_UNIQUE_A(newExpr)  \
	static_assert(std::extent<T>::value == 0, "make_unique forbids T[size]. Please use T[]");  \
	using Elem = typename std::remove_extent<T>::type;  \
	return std::unique_ptr<T>(newExpr)

template<typename T, typename>
inline std::unique_ptr<T>  oel::make_unique(size_t size)
{
	OEL_MAKE_UNIQUE_A( new Elem[size]() ); // value-initialize
}

template<typename T, typename>
inline std::unique_ptr<T>  oel::make_unique_default(size_t size)
{
	OEL_MAKE_UNIQUE_A(new Elem[size]);
}

#undef OEL_MAKE_UNIQUE_A
