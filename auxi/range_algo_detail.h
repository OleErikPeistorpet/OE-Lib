#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail_in_param.h"
#include "impl_algo.h"  // for MemcpyCheck

#include <algorithm>


namespace oel::_detail
{
	template< typename Container, typename Iterator >
	constexpr auto EraseEnd(Container & c, Iterator f)
	->	decltype( c.erase_to_end(f) )
	{	return    c.erase_to_end(f); }

	template< typename Container, typename Iterator, typename... None >
	constexpr void EraseEnd(Container & c, Iterator f, None...)
	{
		c.erase(f, c.end());
	}


	template
	<	typename Container, typename Predicate,
		typename = void
	>
	inline constexpr bool hasRemoveIf = false;

	template< typename Container, typename Predicate >
	inline constexpr bool hasRemoveIf
	<	Container, Predicate,
		std::void_t< decltype( std::declval<Container &>().remove_if(std::declval<Predicate>()) ) >
	>	= true;

	template< typename Predicate, typename Container >
	constexpr auto RemoveIf(Container & c, _detail::InParam<Predicate> p)
	{
		if constexpr( hasRemoveIf<Container, Predicate> )
		{
			return c.remove_if(std::move(p));
		}
		else
		{	_detail::EraseEnd
			(	c,
				std::remove_if( oel::begin_(c), oel::end_(c), std::move(p) )
			);
		}
	}


	template< typename Container >
	constexpr auto Unique(Container & c)
	->	decltype( c.unique() ) { return c.unique(); }

	template< typename Container, typename... None >
	constexpr void Unique(Container & c, None...)
	{
		_detail::EraseEnd( c, std::unique(oel::begin_(c), oel::end_(c)) );
	}

////////////////////////////////////////////////////////////////////////////////

	template< typename InputIter, typename RandomAccessIter >
	InputIter Copy(InputIter src, size_t const n, RandomAccessIter const dest)
	{
		if constexpr( can_memmove_with<RandomAccessIter, InputIter> )
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			if( n != 0 )
			{	// Dereference to detect out of range errors if the iterator has internal check
				(void) *dest;
				(void) *(dest + (n - 1));
			}
		#endif
			_detail::MemcpyCheck(src, n, to_pointer_contiguous(dest));
			return src + n;
		}
		else
		{	for( size_t i{}; i != n; ++i )
			{
				dest[i] = *src;
				++src;
			}
			return src;
		}
	}
}