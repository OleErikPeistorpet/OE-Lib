#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail_assignable.h"
#include "../util.h"  // for TightPair

#include <tuple>

/** @file
*/

namespace oel
{
namespace _detail
{
	template< bool /*CanCallConst*/, typename Func, typename... Iters >
	struct TransformIterBase
	{
		using FnRef = const Func &;

		static constexpr auto canCallConst = true;

		TightPair< std::tuple<Iters...>, typename AssignableWrap<Func>::Type > m;
	};

	template< typename Func, typename... Iters >
	struct TransformIterBase<false, Func, Iters...>
	{
		using FnRef = Func &;

		static constexpr auto canCallConst = false;

		TightPair< std::tuple<Iters...>, typename AssignableWrap<Func>::Type > mutable m;
	};
}


template< bool IsZip, typename Func, typename... Iterators >
class _transformIterator
 :	private _detail::TransformIterBase
	<	std::is_invocable_v< const Func &, decltype(*std::declval<const Iterators &>())... >,
		Func, Iterators...
	>
{
	using _super = typename _transformIterator::TransformIterBase;

	using _iSeq      = std::index_sequence_for<Iterators...>;
	using _firstIter = std::tuple_element_t< 0, std::tuple<Iterators...> >;

	using _super::m;

	static constexpr auto _category()
		{
			if constexpr (std::is_copy_constructible_v<Func> and _super::canCallConst)
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

public:
	using iterator_category = decltype(_transformIterator::_category());

	using difference_type = std::common_type_t< iter_difference_t<Iterators>... >;
	using reference       = decltype( std::declval<typename _super::FnRef>()(*std::declval<const Iterators &>()...) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	_transformIterator() = default;
	constexpr _transformIterator(Func f, Iterators... it)   : _super{{ {std::move(it)...}, std::move(f) }} {}

	//! Return type is `const tuple<Iterators...> &` if IsZip, else the underlying iterator
	constexpr const auto & base() const & noexcept   OEL_ALWAYS_INLINE
		{
			if constexpr (IsZip)
				return m.first;
			else
				return std::get<0>(m.first);
		}
	//! Return type is `tuple<Iterators...>` if IsZip, else the underlying iterator
	constexpr auto base() && noexcept
		{
			static_assert(std::is_nothrow_move_constructible_v< decltype(m.first) >);
			if constexpr (IsZip)
				return std::move(m.first);
			else
				return std::get<0>(std::move(m.first));
		}

	constexpr reference operator*() const   OEL_ALWAYS_INLINE { return _apply(_iSeq{}); }

	constexpr _transformIterator & operator++()
		{
			_increment(_iSeq{});  return *this;
		}
	//! Post-increment: return type is _transformIterator if iterator_category is-a forward_iterator_tag, else void
	constexpr auto operator++(int) &
		{
			if constexpr (std::is_same_v<iterator_category, std::input_iterator_tag>)
			{
				_increment(_iSeq{});
			}
			else
			{	auto tmp = *this;
				_increment(_iSeq{});
				return tmp;
			}
		}
	constexpr _transformIterator & operator--()
		{
			_decrement(_iSeq{});  return *this;
		}
	constexpr _transformIterator   operator--(int) &
		{
			auto tmp = *this;
			_decrement(_iSeq{});
			return tmp;
		}

	constexpr _transformIterator & operator+=(difference_type offset) &
		{
			_advance(offset, _iSeq{});
			return *this;
		}
	constexpr _transformIterator & operator-=(difference_type offset) &
		{
			_advance(-offset, _iSeq{});
			return *this;
		}

	friend constexpr _transformIterator operator +
		(difference_type offset, _transformIterator it)   { return it += offset; }
	[[nodiscard]]  OEL_ALWAYS_INLINE
	friend constexpr _transformIterator operator +
		(_transformIterator it, difference_type offset)   { return it += offset; }
	[[nodiscard]]  OEL_ALWAYS_INLINE
	friend constexpr _transformIterator operator -
		(_transformIterator it, difference_type offset)   { return it -= offset; }

	constexpr difference_type operator -(const _transformIterator & right) const
		OEL_REQUIRES(std::sized_sentinel_for<_firstIter, _firstIter>)
		{
			return std::get<0>(m.first) - std::get<0>(right.m.first);
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, _firstIter>)
	friend constexpr difference_type operator -(_sentinelWrapper<S> left, const _transformIterator & right)
		{
			return left._s - std::get<0>(right.m.first);
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, _firstIter>)
	friend constexpr difference_type operator -(const _transformIterator & left, _sentinelWrapper<S> right)
		{
			return std::get<0>(left.m.first) - right._s;
		}

	constexpr reference operator[](difference_type offset) const
		{
			auto tmp = *this;
			tmp._advance(offset, _iSeq{});
			return *tmp;
		}
	// These are not hidden friends because MSC 2017 gives error C3615
	constexpr bool operator!=(const _transformIterator & right) const
		{
			return std::get<0>(m.first) != std::get<0>(right.m.first);
		}
	constexpr bool operator==(const _transformIterator & right) const
		{
			return std::get<0>(m.first) == std::get<0>(right.m.first);
		}
	constexpr bool operator <(const _transformIterator & right) const
		{
			return std::get<0>(m.first) < std::get<0>(right.m.first);
		}
	constexpr bool operator >(const _transformIterator & right) const   { return right < *this; }

	constexpr bool operator<=(const _transformIterator & right) const   { return !(right < *this); }

	constexpr bool operator>=(const _transformIterator & right) const   { return !(*this < right); }

	template< typename S >
	friend constexpr bool operator!=
		(const _transformIterator & left, _sentinelWrapper<S> right)   { return std::get<0>(left.m.first) != right._s; }

	template< typename S >
	friend constexpr bool operator!=
		(_sentinelWrapper<S> left, const _transformIterator & right)   { return std::get<0>(right.m.first) != left._s; }

	template< typename S >
	friend constexpr bool operator==
		(const _transformIterator & left, _sentinelWrapper<S> right)   { return std::get<0>(left.m.first) == right._s; }

	template< typename S >
	friend constexpr bool operator==
		(_sentinelWrapper<S> left, const _transformIterator & right)   { return right == left; }



////////////////////////////////////////////////////////////////////////////////



private:
	template< size_t... Ns >
	constexpr reference _apply(std::index_sequence<Ns...>) const
	{
		typename _super::FnRef f = m.second();
		return f(*std::get<Ns>(m.first)...);
	}

	template< size_t... Ns >
	OEL_ALWAYS_INLINE constexpr void _increment(std::index_sequence<Ns...>)
	{
		(++std::get<Ns>(m.first), ...);
	}

	template< size_t... Ns >
	OEL_ALWAYS_INLINE constexpr void _decrement(std::index_sequence<Ns...>)
	{
		(--std::get<Ns>(m.first), ...);
	}

	template< size_t... Ns >
	constexpr void _advance(difference_type offset, std::index_sequence<Ns...>)
	{
		((std::get<Ns>(m.first) += offset), ...);
	}
};

#if __cpp_lib_concepts < 201907
	template< bool Z, typename F, typename I0, typename... Is >
	inline constexpr bool disable_sized_sentinel_for
	<	_transformIterator<Z, F, I0, Is...>,
		_transformIterator<Z, F, I0, Is...>
	>	= disable_sized_sentinel_for<I0, I0>;

	template< typename S, bool Z, typename F, typename I0, typename... Is >
	inline constexpr bool disable_sized_sentinel_for< _sentinelWrapper<S>, _transformIterator<Z, F, I0, Is...> >
		= disable_sized_sentinel_for<S, I0>;
#endif

} // namespace oel
