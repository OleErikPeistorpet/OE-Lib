#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "compress_empty.h"


namespace oel
{

//! Similar to boost::transform_iterator
template<typename UnaryFunc, typename Iterator>
class transform_iterator
{
	_detail::TightPair<Iterator, UnaryFunc> _m;

public:
	using iterator_category = typename std::conditional
	<	std::is_base_of< std::forward_iterator_tag, iter_category<Iterator> >::value,
		std::forward_iterator_tag,
		iter_category<Iterator>
	>::type;

	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( _m.second()(*_m.first) );
	using pointer         = void;
	using value_type      = typename std::decay<reference>::type;

	transform_iterator(UnaryFunc f, Iterator it)
	 :	_m{it, f} {
	}

	transform_iterator(transform_iterator &&) = default;
	transform_iterator(const transform_iterator &) = default;

	/** @brief Does not change the UnaryFunc, to support lambda
	*
	* Probably optimizes better than alternatives, but gives potential for surprises  */
	transform_iterator & operator =(const transform_iterator & other) &
	{
		_m.first = other._m.first;
		return *this;
	}

	reference operator*() const
	{
		return _m.second()(*_m.first);
	}

	transform_iterator & operator++()  OEL_ALWAYS_INLINE
	{	// preincrement
		++_m.first;
		return *this;
	}

	transform_iterator operator++(int) &
	{	// postincrement
		auto tmp = *this;
		++_m.first;
		return tmp;
	}

	bool operator==(const transform_iterator & right) const
	{
		return _m.first == right._m.first;
	}

	bool operator!=(const transform_iterator & right) const
	{
		return _m.first != right._m.first;
	}
};

} // namespace oel
