#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "impl_algo.h"

#include <algorithm>

namespace oel
{
namespace _detail
{
	template< typename Container >
	constexpr auto EraseEnd(Container & c, typename Container::iterator f)
	->	decltype(c.erase_to_end(f))
		{ return c.erase_to_end(f); }

	template< typename Container, typename ContainerIter, typename... None >
	constexpr void EraseEnd(Container & c, ContainerIter f, None...) { c.erase(f, c.end()); }


	template< typename Container, typename UnaryPred >
	constexpr auto RemoveIf(Container & c, UnaryPred p, int)
	->	decltype(c.remove_if(p)) { return c.remove_if(p); }

	template< typename Container, typename UnaryPred >
	constexpr void RemoveIf(Container & c, UnaryPred p, long)
	{
		_detail::EraseEnd( c, std::remove_if(begin(c), end(c), p) );
	}

	template< typename Container >
	constexpr auto Unique(Container & c, int)  // pass dummy int to prefer this overload
	->	decltype(c.unique()) { return c.unique(); }

	template< typename Container >
	constexpr void Unique(Container & c, long)
	{
		_detail::EraseEnd( c, std::unique(begin(c), end(c)) );
	}

////////////////////////////////////////////////////////////////////////////////

	template< typename ContiguousIter, typename ContiguousIter2,
	          enable_if< can_memmove_with<ContiguousIter2, ContiguousIter>::value > = 0
	>
	ContiguousIter CopyUnsf(ContiguousIter const src, size_t const n, ContiguousIter2 const dest)
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

	template< typename InputIter, typename RandomAccessIter, typename... None >
	InputIter CopyUnsf(InputIter src, size_t const n, RandomAccessIter const dest, None...)
	{
		for (size_t i{}; i < n; ++i)
		{
			dest[i] = *src;
			++src;
		}
		return src;
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
}

}