#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "impl_algo.h"
#include "../dynarray.h"
#include "../view/counted.h"

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
			_detail::MemcpyCheck(src, n, as_contiguous_address(dest));
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


	template< typename InputRange, typename RandomAccessRange >
	bool CopyFit(InputRange & src, RandomAccessRange & dest)
	{
		if constexpr (range_is_sized<InputRange>)
		{
			auto       n        = as_unsigned(_detail::Size(src));
			auto const destSize = as_unsigned(_detail::Size(dest));
			bool const success{n <= destSize};
			if (!success)
				n = destSize;

			_detail::CopyUnsf(begin(src), n, begin(dest));
			return success;
		}
		else
		{	auto it = begin(src);  auto const last = end(src);
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
	}

////////////////////////////////////////////////////////////////////////////////

	template< typename Alloc, typename... Ranges >
	auto ConcatToDynarr(Alloc a, Ranges &&... rs)
	{
		static_assert((... and rangeIsForwardOrSized<Ranges>));
		using T = std::common_type_t<
				iter_value_t< iterator_t<Ranges> >...
			>;
		size_t const counts[]{_detail::UDist(rs)...};

		size_t sum{};
		for (auto n : counts)
			sum += n;

		auto d = dynarray<T, Alloc>(reserve, sum, std::move(a));

		auto nIt = begin(counts);
		(..., d.append( view::counted(begin(rs), *nIt++) ));

		return d;
	}
}