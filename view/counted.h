#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../auxi/range_traits.h"

/** @file
*/

namespace oel::view
{

//! Wrapper for iterator and size
/**
* Satisfies the std::ranges::range concept only if Iterator is random-access. */
template< typename Iterator >
class counted
{
public:
	using difference_type = iter_difference_t<Iterator>;

	counted() = default;
	constexpr counted(Iterator first, difference_type n)   : _begin{std::move(first)}, _size{n} {}

	constexpr Iterator begin()       { return _detail::MoveIfNotCopyable(_begin); }
	//! Provided only if Iterator is random-access
	template
	<	typename I = Iterator,
		enable_if< iter_is_random_access<I> > = 0
	>
	constexpr Iterator end() const   { return _begin + _size; }

	constexpr auto size() const noexcept   OEL_ALWAYS_INLINE { return std::make_unsigned_t<difference_type>(_size); }

	constexpr bool empty() const noexcept   { return 0 == _size; }

	constexpr decltype(auto) operator[](difference_type index) const
		OEL_REQUIRES(iter_is_random_access<Iterator>)          OEL_ALWAYS_INLINE
		{
			using U = std::make_unsigned_t<difference_type>;
			OEL_ASSERT( static_cast<U>(index) < static_cast<U>(_size) );
			return _begin[index];
		}

private:
	Iterator       _begin;
	difference_type _size;
};

}


#if OEL_STD_RANGES

namespace std::ranges
{

template< typename I >
inline constexpr bool enable_borrowed_range< oel::view::counted<I> > = true;

template< typename I >
inline constexpr bool enable_view< oel::view::counted<I> > = true;

}
#endif
