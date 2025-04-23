#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "subrange.h"

/** @file
*/

namespace oel
{

template< typename Iterator >
class _countedView
{
public:
	using difference_type = std::iter_difference_t<Iterator>;

	constexpr _countedView(Iterator it, difference_type n)   : _begin{std::move(it)}, _size{n} {}

	constexpr Iterator begin() const   { return _begin; }

	constexpr Iterator end() const     { return _begin + _size; }

	OEL_ALWAYS_INLINE
	constexpr auto size() const noexcept    { return std::make_unsigned_t<difference_type>(_size); }

	constexpr bool empty() const noexcept   { return 0 == _size; }

	OEL_ALWAYS_INLINE
	constexpr decltype(auto) operator[](difference_type index) const   { return _begin[index]; }

private:
	Iterator       _begin;
	difference_type _size;
};

namespace view
{

//! Very similar to std::views::counted
inline constexpr auto counted =
	[](auto iterator, auto n)
	{
		if constexpr( iter_is_random_access< decltype(iterator) > )
		{
			return _countedView(std::move(iterator), n);
		}
		else
		{	return subrange(
				std::counted_iterator(std::move(iterator), n),
				std::default_sentinel );
		}
	};

}

} // oel


namespace std::ranges
{

template< typename I >
inline constexpr bool enable_view< oel::_countedView<I> > = true;

template< typename I >
inline constexpr bool enable_borrowed_range< oel::_countedView<I> > = true;

}