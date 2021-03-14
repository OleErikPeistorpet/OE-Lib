#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../type_traits.h"

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
}

}