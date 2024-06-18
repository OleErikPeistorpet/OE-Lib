#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "all.h"
#include "auxi/transform_iterator.h"

/** @file
*/

namespace oel
{
namespace view
{

template< ptrdiff_t N >
struct _adjacentTransformFn
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
//! Similar to std::views::adjacent_transform, same call signature
/**
* Requires that the range is random-access.
*
* https://en.cppreference.com/w/cpp/ranges/adjacent_transform_view  */
template< ptrdiff_t N >
inline constexpr _adjacentTransformFn<N> adjacent_transform{};

} // view

template< typename View, typename Func, ptrdiff_t N >
class _adjacentTransformView
{
	_detail::TightPair< View, typename _detail::AssignableWrap<Func>::Type > _m;

public:
	using difference_type = iter_difference_t< iterator_t<View> >;

	_adjacentTransformView() = default;
	constexpr _adjacentTransformView(View v, Func f)   : _m{std::move(v), std::move(f)} {}

	constexpr auto begin()
		{
			Func & f          = _m.second();
			auto const offset = std::min<difference_type>(_detail::Size(_m.first), N - 1);
			return _iterTransformIterator{
				_wrapFn{_detail::MoveIfNotCopyable(f)},
				_m.first.begin() + offset };
		}
	//! Return type either same as `begin()` or oel::sentinel_wrapper
	template< typename V = View, typename /*EnableIfHasEnd*/ = sentinel_t<V> >
	constexpr auto end()
		{
			if constexpr (std::is_empty_v<Func> and std::is_same_v< iterator_t<V>, sentinel_t<V> >)
				return _iterTransformIterator{ _wrapFn{_m.second()}, _m.first.end() };
			else
				return _sentinelWrapper< sentinel_t<V> >{_m.first.end()};
		}

	constexpr auto size()
		{
			constexpr std::make_unsigned_t<difference_type> min{N - 1};
			auto s = static_cast< decltype(min) >(_detail::Size(_m.first));
			return std::max(s, min) - min;
		}

	constexpr bool empty()     { return size() == 0; }

	constexpr View         base() &&                 { return std::move(_m.first); }
	constexpr const View & base() const & noexcept   { return _m.first; }



////////////////////////////////////////////////////////////////////////////////



private:
	using _fn_7KQw   = Func; // guarding against name collision due to inheritance (MSVC)
	using _it_7KQw   = iterator_t<View>;
	using _iSeq_7KQw = std::make_integer_sequence<ptrdiff_t, N>;

	struct _wrapFn : public Func
	{
		template< ::std::ptrdiff_t... Is >
		OEL_ALWAYS_INLINE constexpr decltype(auto) apply_7KQw
			(_it_7KQw it, ::std::integer_sequence<::std::ptrdiff_t, Is...>) const
		{
			const _fn_7KQw & f = *this;
			return f(it[Is - N + 1]...);
		}

		constexpr decltype(auto) operator()(_it_7KQw it) const
		{
			return apply_7KQw(it, _iSeq_7KQw{});
		}
	};
};

namespace _detail
{
	template< typename F, ptrdiff_t N >
	struct AdjacTransPartial
	{
		F _f;

		template< typename Range >
		friend constexpr auto operator |(Range && r, AdjacTransPartial a)
		{
			auto v = view::all(static_cast<Range &&>(r));
			return _adjacentTransformView< decltype(v), F, N >{std::move(v), std::move(a)._f};
		}

		template< typename Range >
		constexpr auto operator()(Range && r) const
		{
			return static_cast<Range &&>(r) | *this;
		}
	};
}

template< ptrdiff_t N >
template< typename Func >
constexpr auto view::_adjacentTransformFn<N>::operator()(Func f) const
{
	return _detail::AdjacTransPartial<Func, N>{std::move(f)};
}

} // oel

#if OEL_STD_RANGES

namespace std::ranges
{

template< typename V, typename F, std::ptrdiff_t N >
inline constexpr bool enable_borrowed_range< oel::_adjacentTransformView<V, F, N> >
	= enable_borrowed_range< std::remove_cv_t<V> >;

template< typename V, typename F, std::ptrdiff_t N >
inline constexpr bool enable_view< oel::_adjacentTransformView<V, F, N> > = true;

}
#endif
