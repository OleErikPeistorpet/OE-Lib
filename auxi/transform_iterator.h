#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"

#include <boost/compressed_pair.hpp>

#if __cpp_lib_invoke or _HAS_CXX17
#define OEL_HAS_STD_INVOKE  1
#endif

namespace oel
{

//! Similar to boost::transform_iterator
template<typename UnaryFunc, typename Iterator>
struct transform_iterator
{
	boost::compressed_pair<Iterator, UnaryFunc> _m;


	using iterator_category = typename std::conditional
	<	std::is_base_of< forward_traversal_tag, iter_traversal_t<Iterator> >::value,
		forward_traversal_tag,
		iter_traversal_t<Iterator>
	>::type;
	using difference_type = iter_difference_t<Iterator>;
	using reference  =
		#ifdef OEL_HAS_STD_INVOKE
			decltype( std::invoke(_m.second(), *_m.first()) );
		#else
			decltype( _m.second()(*_m.first()) );
		#endif
	using pointer    = void;
	using value_type = typename std::decay<reference>::type;

	transform_iterator(transform_iterator &&) = default;
	transform_iterator(const transform_iterator &) = default;

	/** @brief Does not change the UnaryFunc, to support lambda
	*
	* Probably optimizes better than alternatives, but gives potential for surprises  */
	transform_iterator & operator =(const transform_iterator & other) &
	{
		_m.first() = other._m.first();
		return *this;
	}

	reference operator*() const
	{
	#ifdef OEL_HAS_STD_INVOKE
		return std::invoke(_m.second(), *_m.first());
	#else
		return _m.second()(*_m.first());
	#endif
	}

	transform_iterator & operator++()  OEL_ALWAYS_INLINE
	{	// preincrement
		++_m.first();
		return *this;
	}

	transform_iterator operator++(int) &
	{	// postincrement
		auto tmp = *this;
		++_m.first();
		return tmp;
	}

	bool operator==(const transform_iterator & right) const  OEL_ALWAYS_INLINE
	{
		return _m.first() == right._m.first();
	}

	bool operator!=(const transform_iterator & right) const  OEL_ALWAYS_INLINE
	{
		return _m.first() != right._m.first();
	}
};

} // namespace oel
