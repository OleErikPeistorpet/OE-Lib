#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "transform.h"

#include <array>

/** @file
*/

namespace oel
{
namespace view
{

//! Similar to std::views::adjacent_transform, same call signature
/**
* The resulting view's iterator models merely std::input_iterator
* (for N > 1, unlike std::views::adjacent_transform).
* https://en.cppreference.com/w/cpp/ranges/adjacent_transform_view  */
template< size_t N >
inline constexpr _transformFn<N> adjacent_transform{};

}

template< typename View, typename Func, size_t N >
class _adjacentTransformView
{
	static_assert(iter_is_forward< iterator_t<View> >);

	using _iSeq = std::make_index_sequence<N - 1>;

	_detail::TightPair< View, typename _detail::AssignableWrap<Func>::Type > _m;

public:
	using difference_type = iter_difference_t< iterator_t<View> >;

	_adjacentTransformView() = default;
	constexpr _adjacentTransformView(View v, Func f)   : _m{std::move(v), std::move(f)} {}

	constexpr auto begin()   OEL_ALWAYS_INLINE { return _begin(_iSeq{}); }

	template< typename V = View >  OEL_ALWAYS_INLINE
	constexpr auto end()
	->	sentinel_wrapper< sentinel_t<V> >     { return {_m.first.end()}; }

	template< typename V = View,
	          typename Ret = decltype( std::declval<V>().size() )
	>
	constexpr Ret  size()
		{
			constexpr Ret min = N - 1;
			return std::max(_m.first.size(), min) - min;
		}

	constexpr bool empty()
		{
			static_assert(std::is_copy_constructible_v<Func>);
			return begin() == end();
		}

	constexpr View         base() &&                { return std::move(_m.first); }
	constexpr View         base() const &&          { return _m.first; }
	constexpr const View & base() const & noexcept  { return _m.first; }



////////////////////////////////////////////////////////////////////////////////



private:
	struct _transform
	{
		_detail::TightPair<
			std::array< iterator_t<View>, N - 1 >,
			typename _detail::AssignableWrap<Func>::Type
		>	m;

		template< typename T, size_t... Is >
		OEL_ALWAYS_INLINE constexpr decltype(auto) applyAndIncrement(T && lastVal, std::index_sequence<Is...>)
		{
			Func & f = m.second();
			return f( *(std::get<Is>(m.first)++)..., static_cast<T &&>(lastVal) );
		}

		template< typename T >
		constexpr decltype(auto) operator()(T && tmp)
		{
			return applyAndIncrement(static_cast<T &&>(tmp), _iSeq{});
		}
	};

	template< size_t... Is >
	constexpr auto _begin(std::index_sequence<Is...>)
	{	// Store iterators for [0, N - 1) in _transform, last iterator becomes transform_iterator::base()
		auto    it = _m.first.begin();
		auto const l = _m.first.end();
		return transform_iterator{
			_transform
			{{	{(Is, it != l ? it++ : it)...},
				_detail::MoveIfNotCopyable(_m.second())
			}},
			it };
	}
};

} // oel

#if OEL_STD_RANGES

template< typename V, typename F, std::size_t N >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_adjacentTransformView<V, F, N> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<V> >;

template< typename V, typename F, std::size_t N >
inline constexpr bool std::ranges::enable_view< oel::_adjacentTransformView<V, F, N> > = true;

#endif
