#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "impl_algo.h" // for MemcpyCheck

#include <algorithm>


namespace oel::_detail
{
	template< typename Container, typename Range >
	auto CanAppend(Container & c, Range && src)
	->	decltype( c.append(static_cast<Range &&>(src)), true_type() );

	false_type CanAppend(...);

////////////////////////////////////////////////////////////////////////////////

	template< typename Container, typename Iterator >
	constexpr auto EraseEnd(Container & c, Iterator f)
	->	decltype(c.erase_to_end(f))
	{	return   c.erase_to_end(f); }

	template< typename Container, typename Iterator, typename... None >
	constexpr void EraseEnd(Container & c, Iterator f, None...)
	{
		c.erase(f, c.end());
	}


	template< typename Container, typename UnaryPred >
	constexpr auto RemoveIf(Container & c, UnaryPred p)
	->	decltype( c.remove_if(std::move(p)) )
	{	return    c.remove_if(std::move(p)); }

	template< typename Container, typename UnaryPred, typename... None >
	constexpr void RemoveIf(Container & c, UnaryPred p, None...)
	{
		_detail::EraseEnd
		(	c,
			std::remove_if(begin(c), end(c), std::move(p))
		);
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
	InputIter Copy(InputIter src, size_t const n, RandomAccessIter const dest)
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
		{	for (size_t i{}; i != n; ++i)
			{
				dest[i] = *src;
				++src;
			}
			return src;
		}
	}
}