#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"  // for TightPair
#include "../auxi/assignable.h"

/** @file
*/

namespace oel
{

/** @brief Similar to boost::transform_iterator
*
* But unlike boost::transform_iterator:
* - Has no size overhead for stateless function objects
* - Accepts a lambda as long as any by-value captures are trivially copy constructible and trivially destructible
* - Move-only UnaryFunc and Iterator supported (then transform_iterator itself becomes move-only) */
template< typename UnaryFunc, typename Iterator >
class transform_iterator
{
	_detail::TightPair< Iterator, typename _detail::AssignableWrap<UnaryFunc>::Type > _m;

	static constexpr bool _isBidirectional = iter_is_bidirectional<Iterator>;

public:
	using iterator_category = std::conditional_t<
			std::is_copy_constructible_v<UnaryFunc>,
			std::conditional_t<
				_isBidirectional,
				std::bidirectional_iterator_tag,
				std::conditional_t< iter_is_forward<Iterator>, std::forward_iterator_tag, std::input_iterator_tag >
			>,
			std::input_iterator_tag
		>;
	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( std::declval<UnaryFunc &>()(*_m.first) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	transform_iterator() = default;
	constexpr transform_iterator(UnaryFunc f, Iterator it)   : _m{std::move(it), std::move(f)} {}

	constexpr Iterator         base() &&                       { return std::move(_m.first); }
	constexpr const Iterator & base() const & noexcept  OEL_ALWAYS_INLINE { return _m.first; }

	constexpr reference operator*() const
		OEL_REQUIRES(std::invocable< UnaryFunc const, decltype(*_m.first) >)
		{
			const UnaryFunc & f = _m.second();
			return f(*_m.first);
		}
	constexpr reference operator*()
		{
			UnaryFunc & f = _m.second();
			return f(*_m.first);
		}

	constexpr transform_iterator & operator++()   OEL_ALWAYS_INLINE { ++_m.first;  return *this; }
	//! Post-increment: return type is transform_iterator if iterator_category is-a forward_iterator_tag, else void
	constexpr auto                 operator++(int) &
		{
			if constexpr (iter_is_forward<transform_iterator>)
			{
				auto tmp = *this;
				++_m.first;
				return tmp;
			}
			else
			{	++_m.first;
			}
		}
	constexpr transform_iterator & operator--()   OEL_ALWAYS_INLINE
		OEL_REQUIRES(_isBidirectional)          { --_m.first;  return *this; }

	constexpr transform_iterator   operator--(int) &
		OEL_REQUIRES(_isBidirectional)
		{
			auto tmp = *this;
			--_m.first;
			return tmp;
		}

	constexpr difference_type operator -(const transform_iterator & right) const
		OEL_REQUIRES(std::sized_sentinel_for<Iterator, Iterator>)        { return _m.first - right._m.first; }

	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -(const transform_iterator & left, S right)  { return left._m.first - right; }

	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -(S left, const transform_iterator & right)  { return left - right._m.first; }

	constexpr bool operator==(const transform_iterator & right) const   { return _m.first == right._m.first; }

	constexpr bool operator!=(const transform_iterator & right) const   { return _m.first != right._m.first; }

	template< typename S >
		OEL_REQUIRES(std::sentinel_for<S, Iterator>)
	friend constexpr bool operator==(const transform_iterator & left, S right)   { return left._m.first == right; }

	template< typename S >
		OEL_REQUIRES(std::sentinel_for<S, Iterator>)
	friend constexpr bool operator==(S left, const transform_iterator & right)   { return right._m.first == left; }

	template< typename S >
		OEL_REQUIRES(std::sentinel_for<S, Iterator>)
	friend constexpr bool operator!=(const transform_iterator & left, S right)   { return left._m.first != right; }

	template< typename S >
		OEL_REQUIRES(std::sentinel_for<S, Iterator>)
	friend constexpr bool operator!=(S left, const transform_iterator & right)   { return right._m.first != left; }
};

template< typename F, typename I >
inline constexpr bool disable_sized_sentinel_for< transform_iterator<F, I>, transform_iterator<F, I> >
	= !iter_is_random_access<I>;

} // namespace oel
