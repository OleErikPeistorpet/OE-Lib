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

template< typename Func, typename... Iterators >
class zip_transform_iterator;

namespace view
{
//! Almost same as `std::views::zip_transform(func, view::counted(iterators, count)...)`
/** @param count is the number of elements in the resulting view
*   @param iterators are the beginning of each range to transform together
*
* Unlike std::views::zip_transform, copies or moves the function into the iterator rather than
* storing it just in the view, thus saving one indirection when dereferencing the iterator.
*
* https://en.cppreference.com/w/cpp/ranges/zip_transform_view  */
template< typename Func, typename... Iterators >
class zip_transform_n
{
	using _iter = zip_transform_iterator<Func, Iterators...>;

public:
	using difference_type = iter_difference_t<_iter>;

	constexpr zip_transform_n(Func f, difference_type n, Iterators... is)
		 :	_begin{std::move(f), std::move(is)...}, _size{n} {}

	constexpr _iter begin()   { return _detail::MoveIfNotCopyable(_begin); }
	//! Provided only if first of Iterators is random-access
	template< typename I = _iter,
	          enable_if< iter_is_random_access<typename I::master_iterator> > = 0
	>
	constexpr auto end() const
		{
			return sentinel_wrapper<typename I::master_iterator>{std::get<0>(_begin.base()) + _size};
		}

	constexpr auto size() const noexcept   OEL_ALWAYS_INLINE { return std::make_unsigned_t<difference_type>(_size); }

	constexpr bool empty() const noexcept   { return 0 == _size; }

private:
	_iter          _begin;
	difference_type _size;
};

} // view

template< typename Func, typename... Iterators >
class zip_transform_iterator
{
	using _tuple = std::tuple<Iterators...>;

	_detail::TightPair< _tuple, typename _detail::AssignableWrap<Func>::Type > _m;

public:
	using master_iterator = std::tuple_element_t<0, _tuple>;

	using iterator_category =
		std::conditional_t
		<	std::is_copy_constructible_v<Func>,
			std::conditional_t
			<	(... and iter_is<Iterators, std::forward_iterator_tag>),
				std::forward_iterator_tag,
				std::input_iterator_tag
			>,
			std::input_iterator_tag
		>;
	using difference_type = std::common_type_t< iter_difference_t<Iterators>... >;
	using reference       = decltype( std::declval<const Func &>()(*std::declval<const Iterators &>()...) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	zip_transform_iterator() = default;
	constexpr zip_transform_iterator(Func f, Iterators... is)   : _m{ {std::move(is)...}, std::move(f) } {}

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

	friend constexpr difference_type operator -(const zip_transform_iterator & left, const zip_transform_iterator & right)
		OEL_REQUIRES(std::sized_sentinel_for<master_iterator, master_iterator>)
		{
			return std::get<0>(left._m.first) - std::get<0>(right._m.first);
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, master_iterator>)
	friend constexpr difference_type operator -(sentinel_wrapper<S> left, const zip_transform_iterator & right)
		{
			return left.se - std::get<0>(right._m.first);
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, master_iterator>)
	friend constexpr difference_type operator -(const zip_transform_iterator & left, sentinel_wrapper<S> right)
		{
			return std::get<0>(left._m.first) - right.se;
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
		(const zip_transform_iterator & left, sentinel_wrapper<S> right)   { return std::get<0>(left._m.first) == right.se; }

	template< typename S >
	friend constexpr bool operator==
		(sentinel_wrapper<S> left, const zip_transform_iterator & right)   { return right == left; }

	template< typename S >
	friend constexpr bool operator!=
		(const zip_transform_iterator & left, sentinel_wrapper<S> right)   { return std::get<0>(left._m.first) != right.se; }

	template< typename S >
	friend constexpr bool operator!=
		(sentinel_wrapper<S> left, const zip_transform_iterator & right)   { return right != left; }



////////////////////////////////////////////////////////////////////////////////



private:
	using _iSeq = std::index_sequence_for<Iterators...>;

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

} // oel

template< typename F, typename... Is >
inline constexpr bool oel::enable_view< oel::view::zip_transform_n<F, Is...> > = true;

#if OEL_STD_RANGES

template< typename F, typename... Is >
inline constexpr bool std::ranges::enable_borrowed_range< oel::view::zip_transform_n<F, Is...> > = true;
#endif
