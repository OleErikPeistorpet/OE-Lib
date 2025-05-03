#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "all.h"
#include "../auxi/transform_iterator.h"

/** @file
*/

namespace oel
{
namespace view
{

struct _transformFn
{
	//! Used with operator |
	template< typename UnaryFunc >
	constexpr auto operator()(UnaryFunc f) const;

	template< typename Range, typename UnaryFunc >
	constexpr auto operator()(Range && r, UnaryFunc f) const
		{
			return static_cast<Range &&>(r) | (*this)(std::move(f));
		}
};
//! Similar to std::views::transform, same call signature
/**
* Unlike std::views::transform, copies or moves the function into the iterator rather than
* storing it just in the view, thus saving one indirection when dereferencing the iterator.
* Moreover, the function does not need to model std::regular_invocable if the call operator is not const
* (mutable lambda is fine). Which means it's all right to return different results for the same input.
* This is at least true when used in OE-Lib, and probably anywhere that accepts an input_range.
*
* https://en.cppreference.com/w/cpp/ranges/transform_view  */
inline constexpr _transformFn transform;

} // view


template< typename Func, typename View >
struct _iterTransformView
 :	public _zipTransformView
	<	iter::_iterTransform< Func, iterator_t<View> >,
		Func, View
	>
{
	using _iterTransformView::_zipTransformView::_zipTransformView;

	constexpr View         base() &&                { return std::get<0>(std::move(this->_m.first)); }
	constexpr const View & base() const & noexcept  { return std::get<0>(this->_m.first); }
};

template< typename F, typename V >
_iterTransformView(F, V) -> _iterTransformView<F, V>;



////////////////////////////////////////////////////////////////////////////////
//
// Only implementation details in rest of the file


namespace _detail
{
	template< typename Func_7KQw >
	struct DerefArg : public Func_7KQw
	{
		template< typename T >
		constexpr decltype(auto) operator()(T && arg)
		{
			return static_cast<Func_7KQw &>(*this)( *static_cast<T &&>(arg) );
		}

		template< typename T >
		constexpr auto operator()(T && arg) const
		->	decltype( static_cast<const Func_7KQw &>(*this)( *static_cast<T &&>(arg) ) )
			{  return static_cast<const Func_7KQw &>(*this)( *static_cast<T &&>(arg) ); }
	};


	template< typename F >
	struct TransformPartial
	{
		F _f;

		template< typename Range >
		friend constexpr auto operator |(Range && r, TransformPartial t)
		{
			return _iterTransformView
			{	DerefArg<F>{std::move(t)._f},
				view::all(static_cast<Range &&>(r))
			};
		}

		template< typename Range >
		constexpr auto operator()(Range && r) const
		{
			return static_cast<Range &&>(r) | *this;
		}
	};
}

template< typename UnaryFunc >
constexpr auto view::_transformFn::operator()(UnaryFunc f) const
{
	return _detail::TransformPartial<UnaryFunc>{std::move(f)};
}


template< typename F, typename V >
inline constexpr bool enable_infinite_range< _iterTransformView<F, V> >
	= enable_infinite_range< std::remove_cv_t<V> >;

} // oel

#if OEL_STD_RANGES

namespace std::ranges
{

template< typename F, typename V >
inline constexpr bool enable_borrowed_range< oel::_iterTransformView<F, V> >
	= enable_borrowed_range< std::remove_cv_t<V> >;

template< typename F, typename V >
inline constexpr bool enable_view< oel::_iterTransformView<F, V> > = true;

}
#endif
