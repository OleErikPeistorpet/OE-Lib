#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "subrange.h"

/** @file
*/

namespace oel
{

//! Substitute for boost::counting_iterator
template< typename IntLike >
class iota_iterator
{
	static_assert(std::is_nothrow_copy_constructible_v<IntLike>);

	IntLike _i;

public:
	using iterator_category = std::random_access_iterator_tag;
	using difference_type   = std::common_type_t<ptrdiff_t, decltype( as_signed(_i - _i) )>;
	using value_type        = IntLike;
	using reference         = IntLike;
	using pointer           = void;

	iota_iterator() = default;
	constexpr explicit iota_iterator(IntLike val)   OEL_ALWAYS_INLINE : _i{val} {}

	constexpr reference operator*() const noexcept  OEL_ALWAYS_INLINE { return _i; }

	constexpr iota_iterator & operator++()   OEL_ALWAYS_INLINE { ++_i;  return *this; }

	constexpr iota_iterator & operator--()   OEL_ALWAYS_INLINE { --_i;  return *this; }

	constexpr iota_iterator   operator++(int) &   { return iota_iterator{_i++}; }

	constexpr iota_iterator   operator--(int) &   { return iota_iterator{_i--}; }

	constexpr iota_iterator & operator+=(IntLike offset) &
		{
			_i += offset;  return *this;
		}
	constexpr iota_iterator & operator-=(IntLike offset) &
		{
			_i -= offset;  return *this;
		}
	friend constexpr iota_iterator operator +(IntLike offset, iota_iterator it)  { return it += offset; }
	friend constexpr iota_iterator operator +(iota_iterator it, IntLike offset)  { return it += offset; }

	friend constexpr iota_iterator operator -(iota_iterator it, IntLike offset)  { return it -= offset; }

	constexpr difference_type      operator -(iota_iterator right) const
		{
			return static_cast<difference_type>(_i) - static_cast<difference_type>(right._i);
		}

	constexpr reference operator[](IntLike offset) const   { return _i + offset; }

	constexpr bool      operator==(iota_iterator right) const   { return _i == right._i; }
	constexpr bool      operator!=(iota_iterator right) const   { return _i != right._i; }
	constexpr bool      operator <(iota_iterator right) const   { return _i < right._i; }
	constexpr bool      operator >(iota_iterator right) const   { return _i > right._i; }
	constexpr bool      operator<=(iota_iterator right) const   { return _i <= right._i; }
	constexpr bool      operator>=(iota_iterator right) const   { return _i >= right._i; }
};


namespace view
{
//! Similar to `std::views::iota(beginVal, endVal)`
inline constexpr auto iota =
	[](auto beginVal, auto endVal)
	{
		using I = std::common_type_t< decltype(beginVal), decltype(endVal) >;
		return subrange(iota_iterator<I>(beginVal), iota_iterator<I>(endVal));
	};
}

} // oel
