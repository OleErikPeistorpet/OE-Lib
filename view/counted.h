#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"
#include "detail/core.h"

/** @file
*/

namespace oel
{

//! Wrapper for iterator and size
template< typename Iterator, bool = iter_is_random_access<Iterator> >
class counted_view
{
public:
	using value_type      = iter_value_t<Iterator>;
	using difference_type = iter_difference_t<Iterator>;
	using size_type       = std::make_unsigned_t<difference_type>;

	counted_view() = default;
	constexpr counted_view(Iterator f, difference_type n)   : _begin(f), _size(n) {}
	//! Construct from range (lvalue) that knows its size, with matching iterator type
	template< typename SizedRange,
		enable_if< !std::is_base_of<counted_view, SizedRange>::value > = 0 // avoid being selected for copy
	>
	constexpr counted_view(SizedRange & r)   : _begin(oel::adl_begin(r)), _size(oel::ssize(r)) {}

	constexpr Iterator  begin() const   OEL_ALWAYS_INLINE { return _begin; }
	//! Provided only if Iterator is random-access or special case
	template< typename I = Iterator >
	constexpr auto      end() const
	->	decltype( _detail::SentinelAt<Iterator>::call(std::declval<I>(), difference_type{}) )
		 { return _detail::SentinelAt<Iterator>::call(_begin, _size); }

	constexpr size_type size() const noexcept   OEL_ALWAYS_INLINE { return _size; }

	constexpr bool      empty() const noexcept  OEL_ALWAYS_INLINE { return 0 == _size; }

	//! Modify this view to exclude first element
	constexpr void      drop_front()          { ++_begin; --_size; }
	//! Modify this view to exclude last element
	constexpr void      drop_back() noexcept   { --_size; }

protected:
	Iterator       _begin;
	difference_type _size;
};

//! Specialization for random-access Iterator
template< typename Iterator >
class counted_view<Iterator, true> : public counted_view<Iterator, false>
{
	using _base = counted_view<Iterator, false>;

public:
	using typename _base::value_type;
	using typename _base::difference_type;
	using typename _base::size_type;
	using reference = typename std::iterator_traits<Iterator>::reference;

	using _base::_base;

	constexpr reference back() const        { return this->_begin[this->_size - 1]; }

	constexpr reference operator[](difference_type index) const   OEL_ALWAYS_INLINE { return this->_begin[index]; }
};

namespace view
{

//! Create a counted_view from iterator and count, with type deduced from first
template< typename Iterator >
constexpr counted_view<Iterator> counted(Iterator first, iter_difference_t<Iterator> count)  { return {first, count}; }

}

} // namespace oel

#if OEL_STD_RANGES

template< typename I, bool R >
inline constexpr bool std::ranges::enable_borrowed_range< oel::counted_view<I, R> > = true;

template< typename I, bool R >
inline constexpr bool std::ranges::enable_view< oel::counted_view<I, R> > = true;

#endif
