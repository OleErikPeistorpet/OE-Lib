#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "contiguous_iterator_to_ptr.h"
#include "impl_algo.h"

#include <algorithm>

namespace oel
{
namespace _detail
{
	template< typename Container >
	inline auto EraseEnd(Container & c, typename Container::iterator f)
	->	decltype(c.erase_to_end(f))
		{ return c.erase_to_end(f); }

	template< typename Container, typename ContainerIter, typename... None >
	inline void EraseEnd(Container & c, ContainerIter f, None...) { c.erase(f, c.end()); }


	template< typename Container, typename UnaryPred >
	inline auto RemoveIf(Container & c, UnaryPred p, int)
	->	decltype(c.remove_if(p)) { return c.remove_if(p); }

	template< typename Container, typename UnaryPred >
	inline void RemoveIf(Container & c, UnaryPred p, long)
	{
		_detail::EraseEnd( c, std::remove_if(begin(c), end(c), p) );
	}

	template< typename Container >
	inline auto Unique(Container & c, int)  // pass dummy int to prefer this overload
	->	decltype(c.unique()) { return c.unique(); }

	template< typename Container >
	inline void Unique(Container & c, long)
	{
		_detail::EraseEnd( c, std::unique(begin(c), end(c)) );
	}

////////////////////////////////////////////////////////////////////////////////

	template< typename ContiguousIter, typename ContiguousIter2,
	          enable_if< can_memmove_with<ContiguousIter2, ContiguousIter>::value > = 0
	>
	ContiguousIter CopyUnsf(ContiguousIter const src, std::ptrdiff_t const n, ContiguousIter2 const dest)
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (0 != n)
		{	// Dereference to detect out of range errors if the iterator has internal check
			(void) *dest;
			(void) *(dest + (n - 1));
		}
	#endif
		_detail::MemcpyCheck(src, n, to_pointer_contiguous(dest));
		return src + n;
	}

	template< typename InputIter, typename Integral, typename RandomAccessIter, typename... None >
	InputIter CopyUnsf(InputIter src, Integral const n, RandomAccessIter const dest, None...)
	{
		for (Integral i = 0; i < n; ++i)
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
	->	decltype( oel::ssize(src), bool() ) // better match if ssize(src) is well-formed (SFINAE)
	{
		auto const destSize = oel::ssize(dest);
		auto n = oel::ssize(src);
		bool const success{n <= destSize};
		if (!success)
			n = destSize;

		_detail::CopyUnsf(begin(src), n, begin(dest));
		return success;
	}
}

}