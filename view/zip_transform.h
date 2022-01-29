#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "counted.h"
#include "../util.h"  // for TightPair
#include "../auxi/detail_assignable.h"

#include <tuple>

/** @file
*/

namespace oel
{
namespace view
{
//! TODO
inline constexpr auto zip_transform_n =
	[](auto func, auto count, auto... iterators)
	{
		return counted(zip_transform_iterator{ std::move(func), std::move(iterators)... }, count);
	};
}


template< typename Func, typename... Iterators >
class zip_transform_iterator
{
	using _iSeq      = std::index_sequence_for<Iterators...>;
	using _firstIter = std::tuple_element_t< 0, std::tuple<Iterators...> >;

	_detail::TightPair< std::tuple<Iterators...>, typename _detail::AssignableWrap<Func>::Type > _m;

	static constexpr bool _isBidirectional{(... and iter_is_bidirectional<Iterators>)};

public:
	using iterator_category =
		std::conditional_t<
			std::is_copy_constructible_v<Func>,
			std::conditional_t<
				_isBidirectional,
				std::bidirectional_iterator_tag,
				std::conditional_t<
					(... and iter_is_forward<Iterators>),
					std::forward_iterator_tag,
					std::input_iterator_tag
				>
			>,
			std::input_iterator_tag
		>;
	using difference_type = std::common_type_t< iter_difference_t<Iterators>... >;
	using reference       = decltype( std::declval<const Func &>()(*std::declval<Iterators &>()...) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	zip_transform_iterator() = default;
	constexpr zip_transform_iterator(Func f, Iterators... is)   : _m{ {std::move(is)...}, std::move(f) } {}

	constexpr std::tuple<Iterators...>         base() &&                       { return std::move(_m.first); }
	constexpr std::tuple<Iterators...>         base() const &&                            { return _m.first; }
	constexpr const std::tuple<Iterators...> & base() const & noexcept  OEL_ALWAYS_INLINE { return _m.first; }

	constexpr reference operator*() const   OEL_ALWAYS_INLINE { return _apply(_iSeq{}); }

	constexpr zip_transform_iterator & operator++()
		{
			_increment(_iSeq{});  return *this;
		}
	//! Post-increment: return type is zip_transform_iterator if iterator_category is-a forward_iterator_tag, else void
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
	constexpr zip_transform_iterator & operator--()
		OEL_REQUIRES(_isBidirectional)
		{
			_decrement(_iSeq{});  return *this;
		}
	constexpr zip_transform_iterator   operator--(int) &
		OEL_REQUIRES(_isBidirectional)
		{
			auto tmp = *this;
			_decrement(_iSeq{});
			return tmp;
		}

	friend constexpr difference_type operator -(const zip_transform_iterator & left, const zip_transform_iterator & right)
		OEL_REQUIRES(std::sized_sentinel_for<_firstIter, _firstIter>)
		{
			return std::get<0>(left._m.first) - std::get<0>(right._m.first);
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, _firstIter>)
	friend constexpr difference_type operator -(sentinel_wrapper<S> left, const zip_transform_iterator & right)
		{
			return left._s - std::get<0>(right._m.first);
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, _firstIter>)
	friend constexpr difference_type operator -(const zip_transform_iterator & left, sentinel_wrapper<S> right)
		{
			return std::get<0>(left._m.first) - right._s;
		}

	friend constexpr bool operator==(const zip_transform_iterator & left, const zip_transform_iterator & right)
		{
			return std::get<0>(left._m.first) == std::get<0>(right._m.first);
		}
	friend constexpr bool operator!=(const zip_transform_iterator & left, const zip_transform_iterator & right)
		{
			return std::get<0>(left._m.first) != std::get<0>(right._m.first);
		}
	template< typename S >
	friend constexpr bool operator==
		(const zip_transform_iterator & left, sentinel_wrapper<S> right)   { return std::get<0>(left._m.first) == right._s; }

	template< typename S >
	friend constexpr bool operator==
		(sentinel_wrapper<S> left, const zip_transform_iterator & right)   { return right == left; }

	template< typename S >
	friend constexpr bool operator!=
		(const zip_transform_iterator & left, sentinel_wrapper<S> right)   { return std::get<0>(left._m.first) != right._s; }

	template< typename S >
	friend constexpr bool operator!=
		(sentinel_wrapper<S> left, const zip_transform_iterator & right)   { return std::get<0>(right._m.first) != left._s; }



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
};

#if __cpp_lib_concepts < 201907
	template< typename F, typename I0, typename... Is >
	inline constexpr bool disable_sized_sentinel_for<
		zip_transform_iterator<F, I0, Is...>, zip_transform_iterator<F, I0, Is...>
	>	= disable_sized_sentinel_for<I0, I0>;

	template< typename S, typename F, typename I0, typename... Is >
	inline constexpr bool disable_sized_sentinel_for< sentinel_wrapper<S>, zip_transform_iterator<F, I0, Is...> >
		= disable_sized_sentinel_for<S, I0>;
#endif

} // namespace oel
