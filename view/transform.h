#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "all.h"
#include "transform_iterator.h"

/** @file
*/

namespace oel
{
namespace view
{

template< size_t NAdjacent >
struct _transformFn
{
	//! Used with operator |
	template< typename Func >
	constexpr auto operator()(Func f) const;

	template< typename Range, typename Func >
	constexpr auto operator()(Range && r, Func f) const
		{
			return static_cast<Range &&>(r) | (*this)(std::move(f));
		}
};

/** @brief Similar to std::views::transform, same call signature
*
* Unlike std::views::transform, copies or moves the function into the iterator rather than
* storing it just in the view, thus saving one indirection when dereferencing the iterator.
* Moreover, the function does not need to model std::regular_invocable, meaning it's all right to return different results
* for the same input. This is at least true when used in OE-Lib, and probably anywhere that accepts an input_range.
*
* https://en.cppreference.com/w/cpp/ranges/transform_view  */
inline constexpr _transformFn<1> transform;

} // view

template< typename View, typename Func >
class _transformView
{
	using _iter = transform_iterator< Func, iterator_t<View> >;

	_detail::TightPair< View, typename _detail::AssignableWrap<Func>::Type > _m;

public:
	using difference_type = iter_difference_t<_iter>;

	_transformView() = default;
	constexpr _transformView(View v, Func f)   : _m{std::move(v), std::move(f)} {}

	constexpr _iter begin()
		{
			return {_detail::MoveIfNotCopyable(_m.second()), _m.first.begin()};
		}
	//! Return type either same as `begin()` or oel::sentinel_wrapper
	template< typename V = View, typename /*EnableIfHasEnd*/ = sentinel_t<V> >
	constexpr auto end()
		{
			if constexpr (std::is_empty_v<Func> and std::is_same_v< iterator_t<V>, sentinel_t<V> >)
				return _iter(_m.second(), _m.first.end());
			else
				return sentinel_wrapper< sentinel_t<V> >{_m.first.end()};
		}

	template< typename V = View >  OEL_ALWAYS_INLINE
	constexpr auto size()
	->	decltype( std::declval<V>().size() )  { return _m.first.size(); }

	constexpr bool empty()   { return _m.first.empty(); }

	constexpr View         base() &&                { return std::move(_m.first); }
	constexpr View         base() const &&          { return _m.first; }
	constexpr const View & base() const & noexcept  { return _m.first; }
};



////////////////////////////////////////////////////////////////////////////////



template< typename View, typename Func, size_t N >
class _adjacentTransformView;

namespace _detail
{
	template< typename F, size_t N >
	struct TransformPartial
	{
		F _f;

		template< typename Range >
		friend constexpr auto operator |(Range && r, TransformPartial t)
		{
			auto v = view::all(static_cast<Range &&>(r));
			if constexpr (N <= 1)
				return _transformView{std::move(v), std::move(t)._f};
			else
				return _adjacentTransformView<decltype(v), F, N>{std::move(v), std::move(t)._f};
		}

		template< typename Range >
		constexpr auto operator()(Range && r) const
		{
			return static_cast<Range &&>(r) | *this;
		}
	};
}

template< size_t NAdjacent >
template< typename Func >
constexpr auto view::_transformFn<NAdjacent>::operator()(Func f) const
{
	return _detail::TransformPartial<Func, NAdjacent>{std::move(f)};
}

} // oel

#if OEL_STD_RANGES

template< typename V, typename F >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_transformView<V, F> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<V> >;

template< typename V, typename F >
inline constexpr bool std::ranges::enable_view< oel::_transformView<V, F> > = true;

#endif
