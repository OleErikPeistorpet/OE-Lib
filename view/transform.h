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
	//! Return type either same as `begin()` or sentinel_t<View>
	template< typename V = View, typename /*EnableIfHasEnd*/ = sentinel_t<V> >
	constexpr auto end()
		{
			if constexpr (std::is_empty_v<Func> and std::is_same_v< iterator_t<V>, sentinel_t<V> >)
				return _iter(_m.second(), _m.first.end());
			else
				return _m.first.end();
		}

	template< typename V = View >  OEL_ALWAYS_INLINE
	constexpr auto size()
	->	decltype( std::declval<V>().size() )  { return _m.first.size(); }

	constexpr bool empty()   { return _m.first.empty(); }

	constexpr View         base() &&                { return std::move(_m.first); }
	constexpr const View & base() const & noexcept  { return _m.first; }
};

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	template< typename F >
	struct TransfPartial
	{
		F _f;

		template< typename Range >
		friend constexpr auto operator |(Range && r, TransfPartial t)
		{
			return _transformView{view::all( static_cast<Range &&>(r) ), std::move(t)._f};
		}
	};
}

////////////////////////////////////////////////////////////////////////////////

namespace view
{

struct _transformFn
{
	/** @brief Used with operator | (like std::views)
	@code
	std::bitset<8> arr[] { 3, 5, 7, 11 };
	dynarray<std::string> result( arr | view::transform([](const auto & bs) { return bs.to_string(); }) );
	@endcode  */
	template< typename UnaryFunc >
	constexpr auto operator()(UnaryFunc f) const   { return _detail::TransfPartial<UnaryFunc>{std::move(f)}; }
	//! Like `std::views::transform(r, f)`
	template< typename Range, typename UnaryFunc >
	constexpr auto operator()(Range && r, UnaryFunc f) const
		{
			return static_cast<Range &&>(r) | (*this)(std::move(f));
		}
};
/**
* @brief Given a source range, create a view that transforms each element when dereferenced
*
* Similar to std::views::transform, but moves the function into the iterator rather than
* keeping it just in the view, thus saving one indirection when dereferencing the iterator.
* Moreover, function objects can have non-const `operator()` (such as mutable lambda). */
inline constexpr _transformFn transform;

}

} // namespace oel


#if OEL_STD_RANGES

template< typename V, typename F >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_transformView<V, F> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<V> >;

template< typename V, typename F >
inline constexpr bool std::ranges::enable_view< oel::_transformView<V, F> > = true;

#endif
