#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "impl_algo.h"
#include "../util.h" // for as_unsigned

#include <algorithm>


namespace oel::_detail
{
	template< typename Container, typename Iterator >
	constexpr auto EraseEnd(Container & c, Iterator f)
	->	decltype(c.erase_to_end(f))
	{ 	return   c.erase_to_end(f); }

	template< typename Container, typename Iterator, typename... None >
	constexpr void EraseEnd(Container & c, Iterator f, None...)
	{
		c.erase(f, c.end());
	}


	template< typename Container, typename UnaryPred >
	constexpr auto RemoveIf(Container & c, UnaryPred p)
	->	decltype(c.remove_if(p))
	{	return   c.remove_if(p); }

	template< typename Container, typename UnaryPred, typename... None >
	constexpr void RemoveIf(Container & c, UnaryPred p, None...)
	{
		_detail::EraseEnd( c, std::remove_if(begin(c), end(c), p) );
	}

	template< typename Container >
	constexpr auto Unique(Container & c)
	->	decltype(c.unique()) { return c.unique(); }

	template< typename Container, typename... None >
	constexpr void Unique(Container & c, None...)
	{
		_detail::EraseEnd( c, std::unique(begin(c), end(c)) );
	}

////////////////////////////////////////////////////////////////////////////////

	template< typename InputIter, typename RandomAccessIter >
	InputIter CopyUnsf(InputIter src, size_t const n, RandomAccessIter const dest)
	{
		if constexpr (can_memmove_with<RandomAccessIter, InputIter>)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			if (n != 0)
			{	// Dereference to detect out of range errors if the iterator has internal check
				(void) *dest;
				(void) *(dest + (n - 1));
			}
		#endif
			_detail::MemcpyCheck(src, n, to_pointer_contiguous(dest));
			return src + n;
		}
		else
		{	for (size_t i{}; i < n; ++i)
			{
				dest[i] = *src;
				++src;
			}
			return src;
		}
	}


	template< typename InputRange, typename OutputRange, typename... None >
	bool CopyFit(InputRange & src, OutputRange & dest, None...)
	{
		auto it = begin(src);  auto const last = end(src);
		auto di = begin(dest);  auto const dl = end(dest);
		while (it != last)
		{
			if (di != dl)
			{
				*di = *it;
				++di; ++it;
			}
			else
			{	return false;
			}
		}
		return true;
	}

	template< typename SizedRange, typename RandomAccessRange >
	auto CopyFit(SizedRange & src, RandomAccessRange & dest)
	->	decltype( _detail::Size(src), bool() ) // better match if Size(src) is well-formed (SFINAE)
	{
		auto n              = as_unsigned(_detail::Size(src));
		auto const destSize = as_unsigned(_detail::Size(dest));
		bool const success{n <= destSize};
		if (!success)
			n = destSize;

		_detail::CopyUnsf(begin(src), n, begin(dest));
		return success;
	}

////////////////////////////////////////////////////////////////////////////////

	template< typename Container, typename InputRange >
	constexpr void Append(Container & dest, InputRange && src)
	{
	#if __cpp_concepts >= 201907
		if constexpr (requires (Container c, InputRange r) { c.append_range(r); })
			dest.append_range(static_cast<InputRange &&>(src));
		else
	#endif
			dest.insert(dest.end(), begin(src), end(src));
	}

	template< typename T, typename A, typename InputRange >
	inline void Append(dynarray<T, A> & dest, InputRange && src)
	{
		dest.append(static_cast<InputRange &&>(src));
	}
}