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

		OEL_ALWAYS_INLINE const F & Func() const { return _fun; }
	};

	template< typename Iterator_MSVC_needs_unique_name, typename Empty_function_object_MSVC_name >
	struct TightPair< Iterator_MSVC_needs_unique_name, Empty_function_object_MSVC_name, true >
	 :	Empty_function_object_MSVC_name
	{
		Iterator_MSVC_needs_unique_name inner;

		TightPair(Iterator_MSVC_needs_unique_name it, Empty_function_object_MSVC_name f)
		 :	Empty_function_object_MSVC_name(f), inner(std::move(it)) {
		}

		OEL_ALWAYS_INLINE const Empty_function_object_MSVC_name & Func() const { return *this; }
	};


	template< typename Iter, typename Func,
	          bool = std::is_move_assignable<Func>::value >
	struct TransformIterHelp
	{
		TightPair<Iter, Func> m;

		TransformIterHelp(Func f, Iter it)
		 :	m{std::move(it), std::move(f)} {
		}
	};

	template< typename Iter, typename Func >
	struct TransformIterHelp<Iter, Func, false>
	{
		TightPair<Iter, Func> m;

		TransformIterHelp(Func f, Iter it)
		 :	m{std::move(it), std::move(f)} {
		}

		TransformIterHelp(TransformIterHelp && other)
		 :	m{std::move(other.m.inner), other.m.Func()} {
		}
		TransformIterHelp(const TransformIterHelp &) = default;

		TransformIterHelp & operator =(TransformIterHelp && other)
		{
			m.inner = std::move(other.m.inner);
			return *this;
		}
		TransformIterHelp & operator =(const TransformIterHelp & other)
		{
			m.inner = other.m.inner;
			return *this;
		}
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
	using iterator_category = typename std::conditional<
			std::is_base_of< std::forward_iterator_tag, iter_category<Iterator> >::value
				and std::is_copy_constructible<_impl>::value,
			std::forward_iterator_tag,
			std::input_iterator_tag
		>::type;

	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( _impl::m.Func()(*_impl::m.inner) );
	using pointer         = void;
	using value_type      = typename std::decay<reference>::type;

	using _impl::_impl;

	Iterator base() const &  { return this->m.inner; }
	Iterator base() &&       { return std::move(this->m.inner); }

	reference operator*() const
	{
		return this->m.Func()(*this->m.inner);
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

	template< typename Sentinel >  OEL_ALWAYS_INLINE
	bool operator==(Sentinel right) const  { return this->m.inner == right; }
	template< typename Sentinel >  OEL_ALWAYS_INLINE
	bool operator!=(Sentinel right) const  { return this->m.inner != right; }
};

} // namespace oel
