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
	//! Right-hand side of operator |
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
struct _transformView
{
	View _v;
	OEL_NO_UNIQUE_ADDRESS _detail::MakeAssignable<Func> _f;

	using _iter = _transformIterator< Func, iterator_t<View> >;


	using difference_type = std::iter_difference_t<_iter>;

	constexpr _iter begin()
		{
			return {_detail::MoveIfNotCopyable(_f), _v.begin()};
		}
	//! Return type either same as `begin()` or _sentinelWrapper
	constexpr auto end()
		{
			if constexpr( std::is_empty_v<Func> and std::is_same_v< iterator_t<View>, sentinel_t<View> > )
				return _iter(_f, _v.end());
			else
				return _sentinelWrapper< sentinel_t<View> >{_v.end()};
		}

	OEL_ALWAYS_INLINE
	constexpr auto size()
		requires requires{ _v.size(); }  { return _v.size(); }

	constexpr bool empty()   { return _v.empty(); }

	constexpr decltype(auto) operator[](difference_type index)
		requires requires{ _v[index]; }
		{
			const Func & f = _f;
			return f(_v[index]);
		}

	constexpr View         base() &&                { return std::move(_v); }
	constexpr const View & base() const & noexcept  { return _v; }
};



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename F >
	struct TransformPartial
	#if OEL_HAS_STD_ADAPTOR_CLOSURE
	 :	public ranges::range_adaptor_closure< TransformPartial<F> >
	#endif
	{
		F _f;

		template< typename R >
		constexpr auto operator()(R && range) &&
		{
			using V = _transformView< view::all_t<R>, F >;
			return V{view::all( static_cast<R &&>(range) ), std::move(_f)};
		}

		template< typename R >
		constexpr auto operator()(R && range) const &
		{
			return TransformPartial(*this)( static_cast<R &&>(range) );
		}
	#if !OEL_HAS_STD_ADAPTOR_CLOSURE
		template< typename R >
		friend constexpr auto operator |(R && range, TransformPartial t)
		{
			return std::move(t)( static_cast<R &&>(range) );
		}
	#endif
	};
}

template< typename UnaryFunc >
constexpr auto view::_transformFn::operator()(UnaryFunc f) const
{
	return _detail::TransformPartial<UnaryFunc>{
		#if OEL_HAS_STD_ADAPTOR_CLOSURE
			{},
		#endif
			std::move(f) };
}

} // oel

namespace std::ranges
{

template< typename V, typename F >
inline constexpr bool enable_view< oel::_transformView<V, F> > = true;

template< typename V, typename F >
inline constexpr bool enable_borrowed_range< oel::_transformView<V, F> >
	= enable_borrowed_range< std::remove_cv_t<V> >;

}