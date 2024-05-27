#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "subrange.h"
#include "../auxi/detail_iter_with_func.h"

/** @file
*/

namespace oel
{

struct _defaultSentinel {};


template< typename Generator >
class _generateIterator
 :	private _detail::IterWithFuncBase< ptrdiff_t, Generator, std::is_invocable_v<const Generator &> >
{
	using _base = typename _generateIterator::IterWithFuncBase;

	using _base::m;

public:
	using iterator_category = std::input_iterator_tag;
	using difference_type   = ptrdiff_t;
	using reference         = decltype( std::declval<typename _base::FnRef>()() );
	using pointer           = void;
	using value_type        = std::remove_cv_t< std::remove_reference_t<reference> >;

	constexpr _generateIterator(Generator g, difference_type count)   : _base{{count, std::move(g)}} {}

	constexpr reference operator*() const
		{
			typename _base::FnRef g = m.second();
			return g();
		}

	constexpr _generateIterator & operator++()        OEL_ALWAYS_INLINE { --m.first;  return *this; }

	constexpr void                operator++(int) &   OEL_ALWAYS_INLINE { --m.first; }

	friend constexpr difference_type operator -
		(_defaultSentinel, const _generateIterator & right)  { return right.m.first; }
	friend constexpr difference_type operator -
		(const _generateIterator & left, _defaultSentinel)   { return -left.m.first; }

	friend constexpr bool operator==(const _generateIterator & left, _defaultSentinel)   { return 0 == left.m.first; }

	friend constexpr bool operator==(_defaultSentinel left, const _generateIterator & right)  { return right == left; }

	friend constexpr bool operator!=(const _generateIterator & left, _defaultSentinel)   { return 0 != left.m.first; }

	friend constexpr bool operator!=(_defaultSentinel, const _generateIterator & right)  { return 0 != right.m.first; }
};


namespace view
{
//! Returns a view that generates `count` elements by calling the given generator function
/**
* Almost same as `generate_n` in the Range-v3 library. */
inline constexpr auto generate = [](auto generator, ptrdiff_t count)
	{
		return subrange(_generateIterator{std::move(generator), count}, _defaultSentinel{});
	};
}

} // oel
