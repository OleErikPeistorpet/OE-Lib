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
* Unlike std::views::transform: Copies or moves the function into the iterator rather than
* storing it just in the view, thus saving one indirection when dereferencing the iterator.
* Moreover, the function does not need to model std::regular_invocable if the call operator is not const
* (mutable lambda is fine). Which means it's all right to return different results for the same input.
* Then the iterator satisfies merely std::input_iterator.
*
* https://en.cppreference.com/w/cpp/ranges/transform_view  */
inline constexpr _transformFn transform;

} // view


template< typename View, typename Func >
struct _iterTransformView
{
	_detail::TightPair< View, typename _detail::AssignableWrap<Func>::Type > _m;

	using _iter = _iterTransformIterator< Func, iterator_t<View> >;


	using difference_type = iter_difference_t<_iter>;

	constexpr _iter begin()
		{
			return {_detail::MoveIfNotCopyable(_m.second()), _m.first.begin()};
		}
	//! Return type either same as `begin()` or _sentinelWrapper
	template< typename V = View,
	          typename /*EnableIfHasEnd*/ = sentinel_t<V>
	>
	constexpr auto end()
		{
			if constexpr( std::is_empty_v<Func> and std::is_same_v< iterator_t<V>, sentinel_t<V> > )
				return _iter(_m.second(), _m.first.end());
			else
				return _sentinelWrapper< sentinel_t<V> >{_m.first.end()};
		}

	template< typename V = View >  OEL_ALWAYS_INLINE
	constexpr auto size()
	->	decltype( std::declval<V>().size() )  { return _m.first.size(); }

	constexpr bool empty()   { return _m.first.empty(); }

	constexpr decltype(auto) operator[](difference_type index)
		OEL_REQUIRES(iter_is_random_access< iterator_t<View> >)
		{
			const Func & f = _m.second();
			return f(_m.first.begin() + index);
		}

	constexpr View         base() &&                { return std::move(_m.first); }
	constexpr const View & base() const & noexcept  { return _m.first; }
};



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
		{	return    static_cast<const Func_7KQw &>(*this)( *static_cast<T &&>(arg) ); }
	};


	template< typename F >
	struct TransformPartial
	{
		F _f;

		template< typename R >
		friend constexpr auto operator |(R && range, TransformPartial t)
		{
			auto v = view::all(static_cast<R &&>(range));
			return _iterTransformView< decltype(v), DerefArg<F> >
			{{	std::move(v),
				DerefArg<F>{std::move(t)._f}
			}};
		}

		template< typename R >
		constexpr auto operator()(R && range) const
		{
			return static_cast<R &&>(range) | *this;
		}
	};
}

template< typename UnaryFunc >
constexpr auto view::_transformFn::operator()(UnaryFunc f) const
{
	return _detail::TransformPartial<UnaryFunc>{std::move(f)};
}

} // oel

template< typename V, typename F >
inline constexpr bool oel::enable_view< oel::_iterTransformView<V, F> > = true;

#if OEL_STD_RANGES

template< typename V, typename F >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_iterTransformView<V, F> >
	= enable_borrowed_range< std::remove_cv_t<V> >;
#endif
