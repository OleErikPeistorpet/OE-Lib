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

		OEL_ALWAYS_INLINE const F & func() const noexcept { return _fun; }
	};

	template< typename Iterator_MSVC_needs_unique_name, typename Empty_function_object_MSVC_name >
	struct TightPair< Iterator_MSVC_needs_unique_name, Empty_function_object_MSVC_name, true >
	 :	Empty_function_object_MSVC_name
	{
		Iterator_MSVC_needs_unique_name inner;

		TightPair(Iterator_MSVC_needs_unique_name it, Empty_function_object_MSVC_name f)
		 :	Empty_function_object_MSVC_name(f), inner(it) {
		}

		OEL_ALWAYS_INLINE const Empty_function_object_MSVC_name & func() const noexcept { return *this; }
	};
}


/** @brief Similar to boost::transform_iterator
*
* Note that the transform function is kept for the whole lifetime. It can only be set by
* constructor, and is untouched on assignment. The reason is zero-overhead lambda support  */
template< typename UnaryFunc, typename Iterator >
class transform_iterator
{
	_detail::TightPair<Iterator, UnaryFunc> _m;

public:
	using iterator_category = typename std::conditional<
			std::is_base_of< std::forward_iterator_tag, iter_category<Iterator> >::value,
			std::forward_iterator_tag,
			iter_category<Iterator>
		>::type;

	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( _m.func()(*_m.inner) );
	using pointer         = void;
	using value_type      = typename std::decay<reference>::type;

	Iterator base() const  { return _m.inner; }

	transform_iterator(UnaryFunc f, Iterator it)
	 :	_m{it, f} {
	}

	transform_iterator(const transform_iterator &) = default;

	transform_iterator & operator =(const transform_iterator & other) &
	{
		_m.inner = other._m.inner;
		return *this;
	}

	reference operator*() const
	{
		return _m.func()(*_m.inner);
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

	OEL_ALWAYS_INLINE
	friend bool operator==(const transform_iterator & left, Iterator right)  { return left._m.inner == right; }
	OEL_ALWAYS_INLINE
	friend bool operator!=(const transform_iterator & left, Iterator right)  { return left._m.inner != right; }
	OEL_ALWAYS_INLINE
	friend bool operator==(Iterator left, const transform_iterator & right)  { return left == right._m.inner; }
	OEL_ALWAYS_INLINE
	friend bool operator!=(Iterator left, const transform_iterator & right)  { return left != right._m.inner; }
};

} // namespace oel
