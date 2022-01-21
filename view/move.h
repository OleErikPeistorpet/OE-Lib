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
namespace _detail
{
	template< typename InputView >
	class MoveView
	{
		using _iter = std::move_iterator< iterator_t<InputView> >;

		InputView _base;

	public:
		using difference_type = iter_difference_t<_iter>;

		MoveView() = default;
		constexpr explicit MoveView(InputView v) : _base{std::move(v)} {}

		constexpr _iter begin()
		{
			return _iter{_base.begin()};
		}

		template< typename V = InputView, typename /*EnableIfHasEnd*/ = sentinel_t<V> >
		constexpr auto end()
		{
		#if OEL_STD_RANGES
			using S = std::conditional_t<
					std::is_same_v< iterator_t<V>, sentinel_t<V> >,
					_iter,
					std::move_sentinel< sentinel_t<V> > // What is it good for?
				>;
		#else
			using S = std::move_iterator< sentinel_t<V> >;
		#endif
			return S{_base.end()};
		}

		constexpr bool empty()  { return _base.empty(); }

		template< typename V = InputView >
		OEL_ALWAYS_INLINE constexpr auto size()
		->	decltype( std::declval<V>().size() ) { return _base.size(); }

		OEL_ALWAYS_INLINE constexpr decltype(auto) operator[](difference_type index)
			OEL_REQUIRES(iter_is_random_access<_iter>)  { return begin()[index]; }

		constexpr InputView         base() &&      { return std::move(_base); }
		constexpr const InputView & base() const & { return _base; }
	};

	struct MovePartial
	{
		template< typename InputRange >
		friend constexpr auto operator |(InputRange && r, MovePartial)
		{
			using V = decltype(view::all( static_cast<InputRange &&>(r) ));
			return MoveView<V>{view::all( static_cast<InputRange &&>(r) )};
		}
	};
}


namespace view
{

//! Wrap an input range with std::move_iterator (and conditionally std::move_sentinel), using operator | (like std::views)
constexpr auto move()                  { return _detail::MovePartial{}; }
//! Create a view with std::move_iterator (and conditionally std::move_sentinel) from a range, normal function style
template< typename InputRange >
constexpr auto move(InputRange && r)   { return static_cast<InputRange &&>(r) | _detail::MovePartial{}; }

}

} // oel


#if OEL_STD_RANGES

template< typename V >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_detail::MoveView<V> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<V> >;

template< typename V >
inline constexpr bool std::ranges::enable_view< oel::_detail::MoveView<V> > = true;

#endif
