#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../auxi/view_interface.h"
#if __cpp_lib_concepts >= 201907
#include "subrange.h"
#endif

/** @file
*/

namespace oel
{

template< typename Iterator >
class _countedView
 :	public view_interface< _countedView<Iterator> >
{
public:
	using difference_type = iter_difference_t<Iterator>;

	constexpr _countedView(Iterator it, difference_type n)   : _begin{std::move(it)}, _size{n} {}

	constexpr Iterator begin()       { return _detail::MoveIfNotCopyable(_begin); }
	//! Provided only if Iterator is random-access
#if __cpp_lib_concepts < 201907
	template
	<	typename I = Iterator,
		enable_if< iter_is_random_access<I> > = 0
	>
#endif
	constexpr Iterator end() const   { return _begin + _size; }

	OEL_ALWAYS_INLINE
	constexpr auto size() const noexcept    { return std::make_unsigned_t<difference_type>(_size); }

	constexpr bool empty() const noexcept   { return 0 == _size; }

private:
	Iterator       _begin;
	difference_type _size;
};

namespace view
{

//! Very similar to std::views::counted
/**
* Without C++20: If iterator is not random-access, the view has no `end()` and is meant only for use within OE-Lib. */
inline constexpr auto counted =
	[](auto iterator, auto n)
	{
	#if __cpp_lib_concepts >= 201907
		if constexpr( !iter_is_random_access< decltype(iterator) > )
		{
			return subrange(
				std::counted_iterator(std::move(iterator), n),
				std::default_sentinel );
		}
		else
	#endif
		{	return _countedView(std::move(iterator), n);
		}
	};

}

} // oel


#if OEL_STD_RANGES

template< typename I >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_countedView<I> > = true;

#endif
