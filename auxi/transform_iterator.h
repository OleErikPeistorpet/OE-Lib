#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"


namespace oel
{
namespace _detail
{
	template< typename I, typename F,
		bool = std::is_empty<F>::value >
	struct TightPair
	{
		I inner;
		F _fun;

		OEL_ALWAYS_INLINE const F & Func() const noexcept { return _fun; }
	};

	template<typename Iterator_needs_unique_name_for_MSVC, typename Empty_function_object_named_for_MSVC>
	struct TightPair<Iterator_needs_unique_name_for_MSVC, Empty_function_object_named_for_MSVC, true>
	 :	Empty_function_object_named_for_MSVC
	{
		Iterator_needs_unique_name_for_MSVC inner;

		TightPair(Iterator_needs_unique_name_for_MSVC it, Empty_function_object_named_for_MSVC f)
		 :	Empty_function_object_named_for_MSVC(f), inner(it) {
		}

		OEL_ALWAYS_INLINE const Empty_function_object_named_for_MSVC & Func() const noexcept { return *this; }
	};
}


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
	using reference       = decltype( _m.Func()(*_m.inner) );
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
		_m.inner = other._m.inner;
		return *this;
	}

	reference operator*() const
	{
		return _m.Func()(*_m.inner);
	}

	transform_iterator & operator++()  OEL_ALWAYS_INLINE
	{	// preincrement
		++_m.inner;
		return *this;
	}

	transform_iterator operator++(int) &
	{	// postincrement
		auto tmp = *this;
		++_m.inner;
		return tmp;
	}

	bool operator==(const transform_iterator & right) const
	{
		return _m.inner == right._m.inner;
	}

	bool operator!=(const transform_iterator & right) const
	{
		return _m.inner != right._m.inner;
	}
};

} // namespace oel
