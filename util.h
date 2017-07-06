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



template<typename IteratorSource>
struct copy_unsafe_return
{
	IteratorSource src_last;
};
/**
* @brief Copies the elements in source into the range beginning at dest
* @return begin(source) incremented by source size
*
* If the ranges overlap, behavior is undefined (uses memcpy when possible).
* To move instead of copy, pass view::move(source). To mimic std::copy_n, use view::counted  */
template<typename SizedInputRange, typename RandomAccessIter>
auto copy_unsafe(const SizedInputRange & source, RandomAccessIter dest)
 -> copy_unsafe_return<decltype(begin(source))>;

template<typename IteratorSource, typename IteratorDest>
struct last_iterators
{
	IteratorSource src_last;
	IteratorDest   dest_last;
};
/**
* @brief Copies the elements in source range into dest range, throws std::out_of_range if dest is smaller than source
* @return struct containing begin(source) and begin(dest), both incremented by number of elements in source
*
* The ranges shall not overlap, except if begin(source) equals begin(dest) (self assign).
* To move instead of copy, pass view::move(source)  */
template<typename InputRange, typename RandomAccessRange>
auto copy(const InputRange & source, RandomAccessRange & dest)
 -> last_iterators<decltype(begin(source)), decltype(begin(dest))>;
/**
* @brief Copies as many elements from source as will fit in dest
* @return true if all elements were copied, false means truncation happened
*
* Otherwise same as copy(const InputRange &, RandomAccessRange &)  */
template<typename InputRange, typename RandomAccessRange>
bool copy_fit(const InputRange & source, RandomAccessRange & dest);


///@{
/// Check if index is valid (can be used with operator[]) for array or other range.
template< typename UnsignedInt, typename SizedRange,
          enable_if<std::is_unsigned<UnsignedInt>::value> = 0 > inline
bool index_valid(const SizedRange & r, UnsignedInt index)   { return index < as_unsigned(oel::ssize(r)); }

template<typename SizedRange> inline
bool index_valid(const SizedRange & r, std::int32_t index)  { return 0 <= index && index < oel::ssize(r); }

template<typename SizedRange> inline
bool index_valid(const SizedRange & r, std::int64_t index)
{	// assumes that r.size() is never greater than INT64_MAX
	return static_cast<std::uint64_t>(index) < as_unsigned(oel::ssize(r));
}
///@}


/**
* @brief Same as std::make_unique, but performs direct-list-initialization if there is no matching constructor
*
* (Works for aggregates.) http://open-std.org/JTC1/SC22/WG21/docs/papers/2015/n4462.html  */
template< typename T, typename... Args, typename = enable_if<!std::is_array<T>::value> >
std::unique_ptr<T> make_unique(Args &&... args);

/// Equivalent to std::make_unique (array version).
template< typename T, typename = enable_if<std::is_array<T>::value> >
std::unique_ptr<T> make_unique(size_t arraySize);
/**
* @brief Array is default-initialized, can be significantly faster for non-class elements
*
* Non-class elements get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
template< typename T, typename = enable_if<std::is_array<T>::value> >
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

	template<typename CntigusIter, typename IntT, typename CntigusIter2> inline
	CntigusIter CopyUnsf(CntigusIter const src, IntT const n, CntigusIter2 const dest, true_type)
	{	// can use memcpy
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (0 != n)
		{	// Dereference iterators at bounds, this detects out of range errors if they are checked iterators
			(void) *src; (void) *dest;
			(void) *(dest + (n - 1));
		}
	#endif
		::memcpy(to_pointer_contiguous(dest), to_pointer_contiguous(src), sizeof(*src) * n);
		return src + n;
	}

	template<typename InputIter, typename IntT, typename RanAccessIter>
	InputIter CopyUnsf(InputIter src, IntT const n, RanAccessIter const dest, false_type)
	{
		for (IntT i = 0; i < n; ++i)
		{
			dest[i] = *src;
			++src;
		}
		return src;
	}


#if _MSC_VER
	__declspec(noreturn) inline
#else
	__attribute__((noreturn)) static
#endif
	void ThrowFromCopy()
	{
		OEL_THROW(std::out_of_range("Too small dest for oel::copy"));
	}

	template<typename Ret, typename InputRange, typename OutputRange, typename FuncItersParam, typename FuncNoParam>
	Ret CopyImpl(const InputRange & from, OutputRange & to, FuncItersParam succeed, FuncNoParam fail)
	{
		auto src = begin(from); auto const sLast = end(from);
		auto dest = begin(to);  auto const dLast = end(to);
		while (src != sLast)
		{
			if (dest != dLast)
			{
				*dest = *src;
				++dest; ++src;
			}
			else
			{	return fail();
			}
		}
		return succeed(src, dest);
	}

	template<typename Ret, typename IterSrc, typename IterDest, typename InputRange, typename OutputRange>
	Ret Copy(const InputRange & src, OutputRange & dest, long)
	{
		return _detail::CopyImpl<Ret>
			(	src, dest,
				[](IterSrc s, IterDest d) { return Ret{s, d}; },
				[]() -> Ret { ThrowFromCopy(); }
			);
	}

	template<typename Ret, typename, typename, typename SizedRange, typename RanAccessRange>
	auto Copy(const SizedRange & src, RanAccessRange & dest, int)
	 -> decltype( oel::ssize(src), Ret() )
	{
		auto const n = oel::ssize(src);
		if (n <= oel::ssize(dest))
			return{ oel::copy_unsafe(src, begin(dest)).src_last, begin(dest) + n };
		else
			ThrowFromCopy();
	}

	template<typename InputRange, typename OutputRange> inline
	bool CopyFit(const InputRange & src, OutputRange & dest, long)
	{
		using IterSrc  = decltype(begin(src));
		using IterDest = decltype(begin(dest));
		return _detail::CopyImpl<bool>
			(	src, dest,
				[](IterSrc, IterDest) { return true; },
				[]() { return false; }
			);
	}

	template<typename SizedRange, typename RanAccessRange>
	auto CopyFit(const SizedRange & src, RanAccessRange & dest, int)
	 -> decltype( oel::ssize(src), bool() )
	{
		auto const destSize = oel::ssize(dest);
		auto n = oel::ssize(src);
		bool const success = (n <= destSize);
		if (!success)
			n = destSize;

		oel::copy_unsafe(view::counted(begin(src), n), begin(dest));
		return success;
	}
}
/// @endcond

} // namespace oel

template<typename SizedInputRange, typename RandomAccessIter>
inline auto oel::copy_unsafe(const SizedInputRange & src, RandomAccessIter dest)
 -> copy_unsafe_return<decltype(begin(src))>
{
	using InIter = decltype(begin(src));
	return{ _detail::CopyUnsf(begin(src), oel::ssize(src), dest,
							  can_memmove_with<RandomAccessIter, InIter>()) };
}

template<typename InputRange, typename RandomAccessRange>
inline auto oel::copy(const InputRange & src, RandomAccessRange & dest)
 -> last_iterators<decltype(begin(src)), decltype(begin(dest))>
{
	using IterSrc  = decltype(begin(src));
	using IterDest = decltype(begin(dest));
	return _detail::Copy< last_iterators<IterSrc, IterDest>, IterSrc, IterDest >(src, dest, int{});
}

template<typename InputRange, typename RandomAccessRange>
inline bool oel::copy_fit(const InputRange & src, RandomAccessRange & dest)
{
	return _detail::CopyFit(src, dest, int{});
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
