#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail_assignable.h"
#include "transform_detail.h"
#include "../util.h"  // for TightPair

#include <tuple>


namespace oel
{

template< typename Func, typename... Iterators >
class _zipTransformIterator
{
	using _iSeq      = std::index_sequence_for<Iterators...>;
	using _firstIter = std::tuple_element_t< 0, std::tuple<Iterators...> >;

	_detail::TightPair< std::tuple<Iterators...>, typename _detail::AssignableWrap<Func>::Type > _m;

public:
	using iterator_category = decltype( _detail::TransformIterCat<true, Func, Iterators...>() );

	using difference_type = std::common_type_t< iter_difference_t<Iterators>... >;
	using reference       = decltype( std::declval<const Func &>()(*std::declval<const Iterators &>()...) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	_zipTransformIterator() = default;
	constexpr _zipTransformIterator(Func f, Iterators... it)   : _m{ {std::move(it)...}, std::move(f) } {}

	//! Return type is `const std::tuple<Iterators...> &`
	constexpr const auto & base() const & noexcept   OEL_ALWAYS_INLINE { return _m.first; }
	//! Return type is `std::tuple<Iterators...>`
	constexpr auto         base() && noexcept
		{
			static_assert(std::is_nothrow_move_constructible_v< decltype(_m.first) >);
			return std::move(_m.first);
		}

	constexpr reference operator*() const   OEL_ALWAYS_INLINE { return _apply(_iSeq{}); }

	constexpr _zipTransformIterator & operator++()
		{
			_increment(_iSeq{});  return *this;
		}
	//! Post-increment: return type is _zipTransformIterator if iterator_category is-a forward_iterator_tag, else void
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
	constexpr _zipTransformIterator & operator--()
		{
			_decrement(_iSeq{});  return *this;
		}
	constexpr _zipTransformIterator   operator--(int) &
		{
			auto tmp = *this;
			_decrement(_iSeq{});
			return tmp;
		}

	constexpr _zipTransformIterator & operator+=(difference_type offset) &
		{
			_advance(offset, _iSeq{});
			return *this;
		}
	constexpr _zipTransformIterator & operator-=(difference_type offset) &
		{
			_advance(-offset, _iSeq{});
			return *this;
		}

	friend constexpr _zipTransformIterator operator +
		(difference_type offset, _zipTransformIterator it)   { return it += offset; }
	[[nodiscard]]  OEL_ALWAYS_INLINE
	friend constexpr _zipTransformIterator operator +
		(_zipTransformIterator it, difference_type offset)   { return it += offset; }
	[[nodiscard]]  OEL_ALWAYS_INLINE
	friend constexpr _zipTransformIterator operator -
		(_zipTransformIterator it, difference_type offset)   { return it -= offset; }

	constexpr difference_type operator -(const _zipTransformIterator & right) const
		OEL_REQUIRES(std::sized_sentinel_for<_firstIter, _firstIter>)
		{
			return std::get<0>(_m.first) - std::get<0>(right._m.first);
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, _firstIter>)
	friend constexpr difference_type operator -(_sentinelWrapper<S> left, const _zipTransformIterator & right)
		{
			return left._s - std::get<0>(right._m.first);
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, _firstIter>)
	friend constexpr difference_type operator -(const _zipTransformIterator & left, _sentinelWrapper<S> right)
		{
			return std::get<0>(left._m.first) - right._s;
		}

	constexpr reference operator[](difference_type offset) const
		{
			auto tmp = *this;
			tmp._advance(offset, _iSeq{});
			return *tmp;
		}

	constexpr bool operator!=(const _zipTransformIterator & right) const
		{
			return std::get<0>(_m.first) != std::get<0>(right._m.first);
		}
	constexpr bool operator==(const _zipTransformIterator & right) const
		{
			return std::get<0>(_m.first) == std::get<0>(right._m.first);
		}
	constexpr bool operator <(const _zipTransformIterator & right) const
		{
			return std::get<0>(_m.first) < std::get<0>(right._m.first);
		}
	constexpr bool operator >(const _zipTransformIterator & right) const   { return right < *this; }

	constexpr bool operator<=(const _zipTransformIterator & right) const   { return !(right < *this); }

	constexpr bool operator>=(const _zipTransformIterator & right) const   { return !(*this < right); }

	template< typename S >
	friend constexpr bool operator!=
		(const _zipTransformIterator & left, _sentinelWrapper<S> right)   { return std::get<0>(left._m.first) != right._s; }

	template< typename S >
	friend constexpr bool operator!=
		(_sentinelWrapper<S> left, const _zipTransformIterator & right)   { return std::get<0>(right._m.first) != left._s; }

	template< typename S >
	friend constexpr bool operator==
		(const _zipTransformIterator & left, _sentinelWrapper<S> right)   { return std::get<0>(left._m.first) == right._s; }

	template< typename S >
	friend constexpr bool operator==
		(_sentinelWrapper<S> left, const _zipTransformIterator & right)   { return right == left; }



////////////////////////////////////////////////////////////////////////////////



private:
	template< size_t... Ns >
	constexpr reference _apply(std::index_sequence<Ns...>) const
	{
		const Func & f = _m.second();
		return f(*std::get<Ns>(_m.first)...);
	}

	template< size_t... Ns >
	OEL_ALWAYS_INLINE constexpr void _increment(std::index_sequence<Ns...>)
	{
		(++std::get<Ns>(_m.first), ...);
	}

	template< size_t... Ns >
	OEL_ALWAYS_INLINE constexpr void _decrement(std::index_sequence<Ns...>)
	{
		(--std::get<Ns>(_m.first), ...);
	}

	template< size_t... Ns >
	constexpr void _advance(difference_type offset, std::index_sequence<Ns...>)
	{
		((std::get<Ns>(_m.first) += offset), ...);
	}
};

#if __cpp_lib_concepts < 201907
	template< typename F, typename I0, typename... Is >
	inline constexpr bool disable_sized_sentinel_for
	<	_zipTransformIterator<F, I0, Is...>,
		_zipTransformIterator<F, I0, Is...>
	>	= disable_sized_sentinel_for<I0, I0>;

	template< typename S, typename F, typename I0, typename... Is >
	inline constexpr bool disable_sized_sentinel_for< _sentinelWrapper<S>, _zipTransformIterator<F, I0, Is...> >
		= disable_sized_sentinel_for<S, I0>;
#endif

} // namespace oel
