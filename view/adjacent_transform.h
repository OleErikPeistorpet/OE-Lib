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

template< size_t N, typename Func, typename View >
class _adjacentTransformView
{
	static_assert(iter_is_forward< iterator_t<View> >);

	using _iSeq = std::make_index_sequence<N - 1>;

	_detail::TightPair< View, typename _detail::AssignableWrap<Func>::Type > _m;

	template< size_t... Is >
	constexpr auto _begin(std::index_sequence<Is...>)
		{
			auto    it = _m.first.begin();
			auto const l = _m.first.end();
			Func &     f = _m.second();
			using TI = decltype( make_zip_transform_iter(f, it, (Is, it)...) );
			return TI{
				_detail::MoveIfNotCopyable(_m.second()),
				it,
				(Is, (it != l ? ++it : it))... }; // brace init forces evaluation order
		}
	template< size_t... Is >
	constexpr auto _end(std::index_sequence<Is...>)
		{
			return std::make_tuple(
				(Is, unreachable_sentinel)...,
				_m.first.end() );
		}

public:
	using difference_type = iter_difference_t< iterator_t<View> >;

	_adjacentTransformView() = default;
	constexpr _adjacentTransformView(Func f, View v)   : _m{std::move(v), std::move(f)} {}

	constexpr auto begin()   OEL_ALWAYS_INLINE { return _begin(_iSeq{}); }

	template< typename V = View, typename /*EnableIfHasEnd*/ = sentinel_t<V> >
	constexpr auto end()     OEL_ALWAYS_INLINE { return _end(_iSeq{}); }

	template< typename V = View >
	constexpr auto size()
	->	decltype( std::declval<V>().size() )
		{	// TODO: std::max< decltype(_m.first.size()) > in case not size_t?
			return std::max(_m.first.size(), N - 1) - (N - 1);
		}

	constexpr bool empty()   { return begin() == end(); }

	constexpr View         base() &&                { return std::move(_m.first); }
	constexpr const View & base() const & noexcept  { return _m.first; }
};

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	template< size_t N, typename F >
	struct AdjaTransPartial
	{
		F _f;

		template< typename Range >
		friend constexpr auto operator |(Range && r, AdjaTransPartial a)
		{
			using V =_adjacentTransformView< N, F, decltype(view::all( static_cast<Range &&>(r) )) >;
			return V{std::move(a)._f, view::all( static_cast<Range &&>(r) )};
		}

		template< typename Range >
		constexpr auto operator()(Range && r) const
		{
			return static_cast<Range &&>(r) | *this;
		}
	};
}

////////////////////////////////////////////////////////////////////////////////

namespace view
{

template< size_t N >
struct _adjacentTransformFn
{
	//! Used with operator |
	template< typename Func >
	constexpr auto operator()(Func f) const   { return _detail::AdjaTransPartial<N, Func>{std::move(f)}; }

	template< typename Range, typename Func >
	constexpr auto operator()(Range && r, Func f) const
		{
			return static_cast<Range &&>(r) | (*this)(std::move(f));
		}
};
/**
* @brief Similar to std::views::adjacent_transform, same call signature
*
* Function objects are required to have const `operator()`, unlike std::views::adjacent_transform.
* This is because the function is copied or moved into the iterator rather than storing it
* just in the view, in order to save one indirection when dereferencing the iterator.
* https://en.cppreference.com/w/cpp/ranges/adjacent_transform_view  */
template< size_t N >
inline constexpr _adjacentTransformFn<N> adjacent_transform;

} // view

}


#if OEL_STD_RANGES

template< std::size_t N, typename F, typename V >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_adjacentTransformView<N, F, V> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<V> >;

template< std::size_t N, typename F, typename V >
inline constexpr bool std::ranges::enable_view< oel::_adjacentTransformView<N, F, V> > = true;

#endif
