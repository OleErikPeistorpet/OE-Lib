#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"

#ifdef OEL_NO_BOOST
#include <tuple>
#else
#include <boost/compressed_pair.hpp>
#endif

namespace oel
{

//! Similar to boost::transform_iterator
template<typename UnaryFunc, typename Iterator>
class transform_iterator
{
#ifdef OEL_NO_BOOST
	std::tuple<Iterator, UnaryFunc> _m;

	#define OEL_TRANIT_BASE(m) std::get<0>(m)
	#define OEL_TRANIT_FUN(m)  std::get<1>(m)
#else
	boost::compressed_pair<Iterator, UnaryFunc> _m;

	#define OEL_TRANIT_BASE(m) m.first()
	#define OEL_TRANIT_FUN(m)  m.second()
#endif

public:
	// TODO: check
	using iterator_category = typename std::conditional
	<	std::is_base_of< forward_traversal_tag, iter_traversal_t<Iterator> >::value,
		forward_traversal_tag,
		iter_traversal_t<Iterator>
	>::type;

	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( OEL_TRANIT_FUN(_m)(*OEL_TRANIT_BASE(_m)) );
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
		OEL_TRANIT_BASE(_m) = OEL_TRANIT_BASE(other._m);
		return *this;
	}

	reference operator*() const
	{
		return OEL_TRANIT_FUN(_m)(*OEL_TRANIT_BASE(_m));
	}

	transform_iterator & operator++()  OEL_ALWAYS_INLINE
	{	// preincrement
		++OEL_TRANIT_BASE(_m);
		return *this;
	}

	transform_iterator operator++(int) &
	{	// postincrement
		auto tmp = *this;
		++OEL_TRANIT_BASE(_m);
		return tmp;
	}

	bool operator==(const transform_iterator & right) const  OEL_ALWAYS_INLINE
	{
		return OEL_TRANIT_BASE(_m) == OEL_TRANIT_BASE(right._m);
	}

	bool operator!=(const transform_iterator & right) const  OEL_ALWAYS_INLINE
	{
		return OEL_TRANIT_BASE(_m) != OEL_TRANIT_BASE(right._m);
	}


#undef OEL_TRANIT_FUN
#undef OEL_TRANIT_BASE
};

} // namespace oel
