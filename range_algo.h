#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "range_view.h"

#include <algorithm>
#include <functional> // for equal_to


/** @file
* @brief Efficient range-based erase and copy functions
*
* Designed to interface with the standard library. Also contains non-member assign, append, insert functions.
*/

namespace oel
{

/** @brief Erase the element at index from container without maintaining order of elements after.
*
* Constant complexity (compared to linear in the distance between position and last for standard erase).
* The end iterator and any iterator, pointer and reference referring to the last element may become invalid. */
template<typename RandomAccessContainer> inline
void erase_unstable(RandomAccessContainer & c, typename RandomAccessContainer::size_type index)
{
	c[index] = std::move(c.back());
	c.pop_back();
}

/**
* @brief Erase from container all elements for which predicate returns true
*
* This function mimics feature from paper N4009 (C++17). Wraps std::remove_if  */
template<typename Container, typename UnaryPredicate>
void erase_if(Container & c, UnaryPredicate p);
/**
* @brief Erase consecutive duplicate elements in container. Wraps std::unique
*
* To erase duplicates anywhere, sort container contents first. (Or just use std::set or unordered_set)  */
template< typename Container, typename BinaryPredicate = std::equal_to<typename Container::value_type> >
void erase_successive_dup(Container & c, BinaryPredicate isDup = BinaryPredicate{});



template<typename IteratorSource>
struct copy_unsafe_return
{
	IteratorSource source_last;
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
	IteratorSource source_last;
	IteratorDest   dest_last;
};
/**
* @brief Copies the elements in source range into dest range, throws std::out_of_range if dest is smaller than source
* @return struct containing begin(source) and begin(dest), both incremented by number of elements in source
*
* The ranges shall not overlap, except if `begin(source)` equals `begin(dest)` (self assign).
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
//! For generic code that may use either dynarray or std library container
template<typename Container, typename InputRange> inline
void assign(Container & dest, const InputRange & source)  { dest.assign(begin(source), end(source)); }

template<typename Container, typename InputRange> inline
void append(Container & dest, const InputRange & source)  { dest.insert(dest.end(), begin(source), end(source)); }

template<typename Container, typename T> inline
void append(Container & dest, typename Container::size_type count, const T & val)
{
	dest.resize(dest.size() + count, val);
}

template<typename Container, typename InputRange> inline
typename Container::iterator insert(Container & dest, typename Container::const_iterator pos, const InputRange & source)
{
	return dest.insert(pos, begin(source), end(source));
}
///@}



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


namespace _detail
{
	template<typename Container> inline
	auto EraseEnd(Container & c, typename Container::iterator f)
	 -> decltype(c.erase_to_end(f)) { return c.erase_to_end(f); }

	template<typename Container, typename... None> inline
	void EraseEnd(Container & c, typename Container::iterator f, None...) { c.erase(f, c.end()); }

	template<typename Container, typename UnaryPred> inline
	auto RemoveIf(Container & c, UnaryPred p, int)  // pass dummy int to prefer this overload
	 -> decltype(c.remove_if(p)) { return c.remove_if(p); }

	template<typename Container, typename UnaryPred>
	void RemoveIf(Container & c, UnaryPred p, long)
	{
		_detail::EraseEnd( c, std::remove_if(begin(c), end(c), p) );
	}

	template<typename Container, typename BinaryPred> inline
	auto Unique(Container & c, BinaryPred p, int)
	 -> decltype(c.unique(p)) { return c.unique(p); }

	template<typename Container, typename BinaryPred>
	void Unique(Container & c, BinaryPred p, long)
	{
		_detail::EraseEnd( c, std::unique(begin(c), end(c), p) );
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
		_detail::MemcpyMaybeNull(to_pointer_contiguous(dest), to_pointer_contiguous(src), sizeof(*src) * n);
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


	const char *const errorCopyMsg = "Too small dest oel::copy";

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

	template<typename Ret, typename IterSrc, typename IterDest, typename InputRange, typename OutputRange> inline
	Ret Copy(const InputRange & src, OutputRange & dest, long)
	{
		return _detail::CopyImpl<Ret>
			(	src, dest,
				[](IterSrc si, IterDest di) { return Ret{si, di}; },
				[]() -> Ret { Throw::OutOfRange(errorCopyMsg); }
			);
	}

	template<typename Ret, typename, typename, typename SizedRange, typename RanAccessRange>
	Ret Copy(const SizedRange & src, RanAccessRange & dest, decltype( oel::ssize(src), int() ))
	{
		auto const n = oel::ssize(src);
		if (n <= oel::ssize(dest))
		{
			auto sLast = oel::copy_unsafe(src, begin(dest)).source_last;
			return {sLast, begin(dest) + n};
		}
		else
		{	Throw::OutOfRange(errorCopyMsg);
		}
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
	bool CopyFit(const SizedRange & src, RanAccessRange & dest, decltype( oel::ssize(src), int() ))
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

template<typename Container, typename BinaryPredicate>
inline void oel::erase_successive_dup(Container & c, BinaryPredicate p)
{
	_detail::Unique(c, p, int{});
}