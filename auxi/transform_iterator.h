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
	template< typename F, typename Arg >
	decltype( std::declval<const F &>()(std::declval<Arg>()),
		true_type() ) testConstF(int);

	template< typename, typename >
	false_type testConstF(long);

	template< typename F, typename Arg >
	using IsConstFunc = decltype(testConstF<F, Arg>(0));


	template< typename I, typename F,
	          bool = std::is_empty<F>::value >
	struct TightPair
	{
		I inner;
		F _fun;

		OEL_ALWAYS_INLINE const F & func() const { return _fun; }
		OEL_ALWAYS_INLINE       F & func()       { return _fun; }
	};

	template< typename Iterator_MSVC_needs_unique_name, typename Empty_function_object_MSVC_name >
	struct TightPair< Iterator_MSVC_needs_unique_name, Empty_function_object_MSVC_name, true >
	 :	Empty_function_object_MSVC_name
	{
		Iterator_MSVC_needs_unique_name inner;

		TightPair(Iterator_MSVC_needs_unique_name it, Empty_function_object_MSVC_name f)
		 :	Empty_function_object_MSVC_name(f), inner(it) {
		}

		OEL_ALWAYS_INLINE const Empty_function_object_MSVC_name & func() const { return *this; }
		OEL_ALWAYS_INLINE       Empty_function_object_MSVC_name & func()       { return *this; }
	};


	template< typename Iter, typename Func,
		bool = IsConstFunc< Func, typename std::iterator_traits<Iter>::reference >::value
	>
	struct TransformIterHelp
	{
		using iterator_category = typename std::conditional<
				std::is_base_of< std::forward_iterator_tag, iter_category<Iter> >::value,
				std::forward_iterator_tag,
				iter_category<Iter>
			>::type;

		TightPair<Iter, Func> m;

		TransformIterHelp(Func f, Iter it) : m{it, f} {}

		TransformIterHelp(const TransformIterHelp &) = default;

		TransformIterHelp & operator =(const TransformIterHelp & other)
		{	// skip m.func(), likely non-assignable
			m.inner = other.m.inner;
			return *this;
		}
	};

	template< typename Iter, typename Func >
	struct TransformIterHelp<Iter, Func, false>
	{	// Func object may modify itself, so we must present a single-pass iterator
		using iterator_category = std::input_iterator_tag;

		TightPair<Iter, Func> m;

		TransformIterHelp(Func f, Iter it) : m{it, f} {}
	};
}


/** @brief Similar to boost::transform_iterator
*
* Note that the transform function to use must be set at construction,
* assignment may leave it untouched. The reason is zero-overhead lambda support  */
template< typename UnaryFunc, typename Iterator >
class transform_iterator  : private _detail::TransformIterHelp<Iterator, UnaryFunc>
{
	using _impl = _detail::TransformIterHelp<Iterator, UnaryFunc>;

public:
	using iterator_category = typename _impl::iterator_category;

	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( _impl::m.func()(*_impl::m.inner) );
	using pointer         = void;
	using value_type      = typename std::decay<reference>::type;

	using _impl::_impl;

	Iterator base() const  { return this->m.inner; }

	reference operator*() const
	{
		return this->m.func()(*this->m.inner);
	}
	reference operator*()
	{
		return this->m.func()(*this->m.inner);
	}

	transform_iterator & operator++()  OEL_ALWAYS_INLINE
	{	// preincrement
		++this->m.inner;
		return *this;
	}

	transform_iterator operator++(int) &
	{	// postincrement
		auto tmp = *this;
		++this->m.inner;
		return tmp;
	}

	bool operator==(Iterator right) const  OEL_ALWAYS_INLINE { return this->m.inner == right; }
	bool operator!=(Iterator right) const  OEL_ALWAYS_INLINE { return this->m.inner != right; }
};

} // namespace oel
