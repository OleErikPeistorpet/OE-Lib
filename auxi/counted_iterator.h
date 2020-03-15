#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"


namespace oel
{

struct counted_sentinel {};

/** @brief
*
*  */
template< typename Iterator >
class counted_iterator
{
public:
	using iterator_category = typename std::conditional<
			std::is_base_of< std::forward_iterator_tag, iter_category<Iterator> >::value,
			std::forward_iterator_tag,
			iter_category<Iterator>
		>::type;

	using difference_type = iter_difference_t<Iterator>;
	using value_type      = iter_value_t<Iterator>;
	using reference       = typename std::iterator_traits<Iterator>::reference;
	using pointer         = void;

	constexpr counted_iterator() = default;

	constexpr counted_iterator(Iterator it, difference_type n)
	 :	_inner{std::move(it)}, _n{n}
	{
		OEL_ASSERT(n >= 0);
	}

	constexpr Iterator base() const &  { return _inner; }
	constexpr Iterator base() &&       { return std::move(_inner); }

	constexpr reference operator*() const  OEL_ALWAYS_INLINE { return *_inner; }

	constexpr counted_iterator & operator++()
	{	// preincrement
		++_inner;
		--_n;
		return *this;
	}

	constexpr counted_iterator operator++(int) &  OEL_ALWAYS_INLINE
	{	// postincrement
		auto tmp = *this;
		++(*this);
		return tmp;
	}

	constexpr bool operator==(counted_sentinel right) const  OEL_ALWAYS_INLINE { return 0 == _n; }
	constexpr bool operator!=(counted_sentinel right) const  OEL_ALWAYS_INLINE { return 0 != _n; }

private:
	Iterator    _inner;
	difference_type _n;
};

} // namespace oel
