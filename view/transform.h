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

/** @brief Given a source range, transform each element when dereferenced, using operator | (like std::views)
@code
std::bitset<8> arr[] { 3, 5, 7, 11 };
dynarray<std::string> result( arr | view::transform([](const auto & bs) { return bs.to_string(); }) );
@endcode
* Unlike std::views::transform, copies or moves the function into the iterator rather than
* storing it just in the view, thus saving one indirection when dereferencing the iterator.
* When used in OE-Lib, the function does not need to model std::regular_invocable,
* meaning it's all right to return different results for the same input. */
template< typename UnaryFunc >
constexpr auto transform(UnaryFunc f);
/*
* @brief Similar to `std::views::transform(r, f)`
*
* See view::transform(UnaryFunc) above. */
template< typename UnaryFunc, typename Range >
constexpr auto transform(Range && r, UnaryFunc f)  { return static_cast<Range &&>(r) | view::transform(std::move(f)); }

}

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	template< typename View, typename Func >
	class TransformView
	{
		using _iter = transform_iterator< Func, iterator_t<View> >;

		TightPair< View, typename _detail::AssignableWrap<Func>::Type > _m;

		template< typename F_ = Func,
		          enable_if< std::is_empty<F_>::value > = 0
		>
		constexpr _iter _makeSent(iterator_t<View> last)
		{
			return {_m.second(), last};
		}

		template< typename Sentinel, typename... None >
		constexpr sentinel_wrapper<Sentinel> _makeSent(Sentinel last, None...)
		{
			return {last};
		}

	public:
		using difference_type = iter_difference_t<_iter>;

		TransformView() = default;

		constexpr TransformView(View v, Func f)
		 :	_m{std::move(v), std::move(f)} {}

		constexpr _iter begin()
		{
			return {_detail::MoveIfNotCopyable( _m.second() ), _m.first.begin()};
		}

		template< typename V_ = View, typename /*EnableIfHasEnd*/ = sentinel_t<V_> >
		OEL_ALWAYS_INLINE constexpr auto end()
		{
			return _makeSent(_m.first.end());
		}

		constexpr bool empty()  { return _m.first.empty(); }

		template< typename V_ = View >
		OEL_ALWAYS_INLINE constexpr auto size()
		->	decltype( std::declval<V_>().size() ) { return _m.first.size(); }

		constexpr View         base() &&      { return std::move(_m.first); }
		constexpr const View & base() const & { return _m.first; }
	};

	template< typename F >
	struct TransfPartial
	{
		F _f;

		template< typename Range >
		friend constexpr auto operator |(Range && r, TransfPartial t)
		{
			using V = decltype(view::all( static_cast<Range &&>(r) ));
			return TransformView<V, F>{view::all( static_cast<Range &&>(r) ), std::move(t)._f};
		}
	};
}

template< typename UnaryFunc >
constexpr auto view::transform(UnaryFunc f)
{
	return _detail::TransfPartial<UnaryFunc>{std::move(f)};
}

} // namespace oel

#if OEL_STD_RANGES

template< typename V, typename F >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_detail::TransformView<V, F> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<V> >;

template< typename V, typename F >
inline constexpr bool std::ranges::enable_view< oel::_detail::TransformView<V, F> > = true;

#endif
