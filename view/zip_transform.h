#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "all.h"
#include "transform_iterator.h"

/** @file
*/

namespace oel
{

template< bool IsZip, typename Func, typename... Views >
class _transformView
{
	using _iter = transform_iterator< IsZip, Func, iterator_t<Views>... >;
	using _iSeq = std::index_sequence_for<Views...>;

	_detail::TightPair< tuple<Views...>, Func > _m;

	template< size_t... Ns >
	constexpr auto _empty(std::index_sequence<Ns...>)
		{
			return (... or std::get<Ns>(_m.first).empty());
		}

	template< typename R >
	static constexpr auto _size(R & r)
	->	decltype(r.size()) { return r.size(); }

	template< typename R,
		enable_if< enable_infinite_range<R> > = 0
	>
	static constexpr auto _size(R &)  { return size_t(-1); }
	// FIXME: incorrect if all are unbounded
	template< typename Tuple, size_t... Ns >
	static constexpr auto _mySize(Tuple & views, std::index_sequence<Ns...>)
	->	decltype( std::min({_size(std::get<Ns>(views))...}) )
		 { return std::min({_size(std::get<Ns>(views))...}); }

	template< size_t... Ns >
	constexpr _iter _begin(std::index_sequence<Ns...>)
		{
			return {_detail::MoveIfNotCopyable(_m.second()), std::get<Ns>(_m.first).begin()...};
		}
	template< typename... Vs, size_t... Ns,
	          typename /*EnableIfHasEnd*/ = tuple< sentinel_t<Vs>... >
	>
	constexpr auto _end(tuple<Vs...> & views, std::index_sequence<Ns...>) const
		{
			if constexpr( (... and std::is_same_v< iterator_t<Vs>, sentinel_t<Vs> >)
			              and std::is_empty_v<Func> )
				return _iter{_m.second(), std::get<Ns>(views).end()...};
			else
				return std::make_tuple(std::get<Ns>(views).end()...);
		}
	// TODO: optimize iteration for case where all are sized

public:
	using difference_type = iter_difference_t<_iter>;

	_transformView() = default;
	constexpr _transformView(Func f, Views... vs)   : _m{ {std::move(vs)...}, std::move(f) } {}

	constexpr _iter begin()   OEL_ALWAYS_INLINE { return _begin(_iSeq{}); }
	//! Return type either same as begin() or `tuple< sentinel_t<Views>... >`
	template< typename T = tuple<Views...> >  OEL_ALWAYS_INLINE
	constexpr auto  end()
	->	decltype( _end(std::declval<T &>(), _iSeq{}) )  { return _end(_m.first, _iSeq{}); }

	template< typename T = tuple<Views...> >  OEL_ALWAYS_INLINE
	constexpr auto size()
	->	decltype( _mySize(std::declval<T &>(), _iSeq{}) )  { return _mySize(_m.first, _iSeq{}); }

	constexpr bool empty()   OEL_ALWAYS_INLINE { return _empty(_iSeq{}); }

	constexpr auto         base() &&
		OEL_REQUIRES(!IsZip)    { return std::get<0>(std::move(_m.first)); }
	constexpr const auto & base() const & noexcept
		OEL_REQUIRES(!IsZip)    { return std::get<0>(_m.first); }
};

namespace view
{

//! TODO
inline constexpr auto zip_transform =
	[](auto func, auto &&... ranges)
	{
		using V = _transformView< true, decltype(func), decltype( all(static_cast<decltype(ranges)>(ranges)) )... >;
		return V{std::move(func), all( static_cast<decltype(ranges)>(ranges) )...};
	};
//! TODO
inline constexpr auto zip_transform_n =
	[](auto func, auto count, auto... iterators)
	{
		return counted(make_zip_transform_iter( std::move(func), std::move(iterators)... ), count);
	};

} // view

template< bool Z, typename F, typename... Vs >
inline constexpr bool enable_infinite_range< _transformView<Z, F, Vs...> >
	= (... and enable_infinite_range< std::remove_cv_t<Vs> >);

}

#if OEL_STD_RANGES

template< bool Z, typename F, typename... Vs >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_transformView<Z, F, Vs...> >
	= (... and std::ranges::enable_borrowed_range< std::remove_cv_t<Vs> >);

template< bool Z, typename F, typename... Vs >
inline constexpr bool std::ranges::enable_view< oel::_transformView<Z, F, Vs...> > = true;

#endif
