#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "impl_algo.h"
#include "../view/counted.h"

#include <algorithm>


namespace oel::_detail
{
	template< typename Container, typename Range >
	auto CanAppend(Container & c, Range && src)
	->	decltype( c.append(static_cast<Range &&>(src)), true_type() );

	false_type CanAppend(...);


	template< typename Container >
	auto HasUnorderedErase(Container & c) -> decltype( c.unordered_erase(c.begin()), true_type() );

	false_type HasUnorderedErase(...);

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

		auto d = dynarray<T>(reserve, sum, std::move(a));

		auto nIt = begin(counts);
		(..., d.append( view::counted(begin(rs), *nIt++) ));

		return d;
	}
}