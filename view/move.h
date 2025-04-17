#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "all.h"

/** @file
*/

namespace oel
{

template< typename View >
class _moveView
{
	View _base;

public:
	using difference_type = iter_difference_t< iterator_t<View> >;

	_moveView() = default;
	constexpr explicit _moveView(View v)   : _base{std::move(v)} {}

	constexpr auto begin()   { return std::move_iterator{_base.begin()}; }

	template< typename V = View, typename /*EnableIfHasEnd*/ = sentinel_t<V> >
	constexpr auto end()
		{
		#if OEL_HAS_STD_MOVE_SENTINEL
			if constexpr (!std::is_same_v< iterator_t<V>, sentinel_t<V> >)
				return std::move_sentinel{_base.end()};
			else
		#endif
				return std::move_iterator{_base.end()};
		}

	template< typename V = View >  OEL_ALWAYS_INLINE
	constexpr auto size()
	->	decltype( std::declval<V>().size() )  { return _base.size(); }

	constexpr bool empty()   { return _base.empty(); }

	constexpr decltype(auto) operator[](difference_type index)
		OEL_REQUIRES(iter_is_random_access< iterator_t<View> >)
		{
			return std::move_iterator{_base.begin()}[index];
		}

	constexpr View         base() &&                { return std::move(_base); }
	constexpr View         base() const &&          { return _base; }
	constexpr const View & base() const & noexcept  { return _base; }
};

namespace view
{

struct _moveFn
{
	template< typename InputRange >
	friend constexpr auto operator |(InputRange && r, _moveFn)
		{
			return _moveView{all( static_cast<InputRange &&>(r) )};
		}

	template< typename InputRange >
	constexpr auto operator()(InputRange && r) const   { return static_cast<InputRange &&>(r) | _moveFn{}; }
};
/** @brief Very similar to views::move in the Range-v3 library and std::views::as_rvalue
@code
std::string moveFrom[2] {"abc", "def"};
oel::dynarray movedStrings(moveFrom | view::move);
@endcode  */
inline constexpr _moveFn move;

}

} // oel


#if OEL_STD_RANGES

template< typename V >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_moveView<V> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<V> >;

template< typename V >
inline constexpr bool std::ranges::enable_view< oel::_moveView<V> > = true;

#endif
