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
		InputView _base;

	public:
		using difference_type = iter_difference_t< iterator_t<InputView> >;

		MoveView() = default;
		constexpr explicit MoveView(InputView v) : _base{std::move(v)} {}

		constexpr auto begin()
		{
			return std::move_iterator{_base.begin()};
		}

		template< typename V = InputView, typename /*EnableIfHasEnd*/ = sentinel_t<V> >
		constexpr auto end()
		{
		#if __cpp_lib_concepts >= 201907
			if constexpr (!std::is_same_v< iterator_t<V>, sentinel_t<V> >)
				return std::move_sentinel{_base.end()}; // What is it good for?
			else
		#endif
				return std::move_iterator{_base.end()};
		}

		constexpr bool empty()  { return _base.empty(); }

		template< typename V = InputView >
		OEL_ALWAYS_INLINE constexpr auto size()
		->	decltype( std::declval<V>().size() ) { return _base.size(); }

		OEL_ALWAYS_INLINE constexpr decltype(auto) operator[](difference_type index)
			OEL_REQUIRES(iter_is_random_access< iterator_t<InputView> >)  { return begin()[index]; }

		constexpr InputView         base() &&               { return std::move(_base); }
		constexpr const InputView & base() const & noexcept { return _base; }
	};

	struct MovePartial
	{
		template< typename InputRange >
		friend constexpr auto operator |(InputRange && r, MovePartial)
		{
			return MoveView{view::all( static_cast<InputRange &&>(r) )};
		}
	};
}


namespace view
{

struct _moveFn
{
	/** @brief Create view, for chaining like std::views
	@code
	std::string moveFrom[2] {"abc", "def"};
	dynarray movedStrings(moveFrom | view::move);
	@endcode  */
	template< typename InputRange >
	friend constexpr auto operator |(InputRange && r, _moveFn)
		{
			return _detail::MoveView{view::all( static_cast<InputRange &&>(r) )};
		}
	//! Same as `std::views::as_rvalue(r)` (C++23)
	template< typename InputRange >
	constexpr auto operator()(InputRange && r) const   { return static_cast<InputRange &&>(r) | _moveFn{}; }
};
//! Very similar to views::move in the Range-v3 library and std::views::as_rvalue
inline constexpr _moveFn move;

}

} // oel


#if OEL_STD_RANGES

template< typename V >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_detail::MoveView<V> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<V> >;

template< typename V >
inline constexpr bool std::ranges::enable_view< oel::_detail::MoveView<V> > = true;

#endif
