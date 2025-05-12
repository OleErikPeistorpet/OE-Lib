#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail_assignable.h"

#include <tuple>


namespace oel
{

template< typename TransformIter, typename Func, typename... Views >
class _zipTransformView
{
protected:
	using _iSeq  = std::index_sequence_for<Views...>;
	using _tuple = std::tuple<Views...>;

	OEL_NO_UNIQUE_ADDRESS typename _detail::AssignableWrap<Func>::Type _fn;
	_tuple _views;

	template< size_t... Ns >
	constexpr TransformIter _begin(std::index_sequence<Ns...>)
		{
			return {_detail::MoveIfNotCopyable(_fn), std::get<Ns>(_views).begin()...};
		}
	template
	<	typename... Vs, size_t... Ns,
		typename /*EnableIfHasEnd*/ = std::tuple< sentinel_t<Vs>... >
	>
	constexpr auto _end(std::tuple<Vs...> & views, std::index_sequence<Ns...>) const
		{
			if constexpr( (... and std::is_same_v< iterator_t<Vs>, sentinel_t<Vs> >)
			              and std::is_empty_v<Func> )
			{
				return TransformIter(_fn, std::get<Ns>(views).end()...);
			}
			else
			{	using S = _sentinelWrapper< decltype(std::get<0>(views).end()) >;
				return S{std::get<0>(views).end()};
			}
		}

public:
	using difference_type = iter_difference_t<TransformIter>;

	_zipTransformView() = default;
	constexpr _zipTransformView(Func f, Views... vs)   : _views{std::move(vs)...}, _fn{std::move(f)} {}

	constexpr auto begin()                      { return _begin(_iSeq{}); }
	//! Return type either same as `begin()` or `oel::_sentinelWrapper< sentinel_t<V> >`, where V is first of Views
	template< typename T = _tuple >
	constexpr auto end()
	->	decltype( _end(std::declval<T &>(), _iSeq{}) )  { return _end(_views, _iSeq{}); }

	template< typename V = std::tuple_element_t<0, _tuple> >
	constexpr auto size()
	->	decltype( std::declval<V>().size() )
		{
			return std::get<0>(_views).size();
		}

	constexpr bool empty()
		{
			return std::get<0>(_views).empty();
		}

	constexpr decltype(auto) operator[](difference_type index)
		OEL_REQUIRES(iter_is_random_access<TransformIter>)
		{
			return _begin(_iSeq{})[index];
		}
};



////////////////////////////////////////////////////////////////////////////////



namespace iter
{
	template< bool CanCallConst, typename Func, typename... Iterators >
	constexpr auto _transformCategory()
	{
		if constexpr (std::is_copy_constructible_v<Func> and CanCallConst)
		{
			if constexpr ((... and iter_is_random_access<Iterators>))
				return std::random_access_iterator_tag{};
			else if constexpr ((... and iter_is_bidirectional<Iterators>))
				return std::bidirectional_iterator_tag{};
			else if constexpr ((... and iter_is_forward<Iterators>))
				return std::forward_iterator_tag{};
			else
				return std::input_iterator_tag{};
		}
		else
		{	return std::input_iterator_tag{};
		}
	}
}


template< typename I, typename F, typename... V >
inline constexpr bool enable_infinite_range< _zipTransformView<I, F, V...> >
	= (... and enable_infinite_range< std::remove_cv_t<V> >);

} // oel

#if OEL_STD_RANGES

namespace std::ranges
{

template< typename I, typename F, typename... V >
inline constexpr bool enable_borrowed_range< oel::_zipTransformView<I, F, V...> >
	= (... and enable_borrowed_range< std::remove_cv_t<V> >);

template< typename I, typename F, typename... V >
inline constexpr bool enable_view< oel::_zipTransformView<I, F, V...> > = true;

}
#endif
