#pragma once

// Copyright 2023 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "range_traits.h" // for _sentinelWrapper


namespace oel
{

template< typename DerivedIter, typename DiffT >
struct _iteratorFacade
{
	using difference_type = DiffT;

	constexpr DerivedIter & operator-=(difference_type offset) &  OEL_ALWAYS_INLINE
		{
			return static_cast<DerivedIter &>(*this) += -offset;
		}

	friend constexpr DerivedIter operator +(difference_type offset, DerivedIter it)  { return it += offset; }
	[[nodiscard]]  OEL_ALWAYS_INLINE
	friend constexpr DerivedIter operator +(DerivedIter it, difference_type offset)  { return it += offset; }
	[[nodiscard]]  OEL_ALWAYS_INLINE
	friend constexpr DerivedIter operator -(DerivedIter it, difference_type offset)  { return it += -offset; }

	constexpr decltype(auto) operator[](difference_type offset) const
		{
			auto tmp = static_cast<const DerivedIter &>(*this);
			tmp += offset;
			return *tmp;
		}

	OEL_ALWAYS_INLINE
	friend constexpr bool operator==(const DerivedIter & left, const DerivedIter & right)  { return !(left != right); }
	OEL_ALWAYS_INLINE
	friend constexpr bool operator >(const DerivedIter & left, const DerivedIter & right)  { return right < left; }

	friend constexpr bool operator<=(const DerivedIter & left, const DerivedIter & right)  { return !(right < left); }

	friend constexpr bool operator>=(const DerivedIter & left, const DerivedIter & right)  { return !(left < right); }

	template< typename S >  OEL_ALWAYS_INLINE
	friend constexpr bool operator!=(_sentinelWrapper<S> left, const DerivedIter & right)  { return right != left; }

	template< typename S >  OEL_ALWAYS_INLINE
	friend constexpr bool operator==(const DerivedIter & left, _sentinelWrapper<S> right)  { return !(left != right); }

	template< typename S >
	friend constexpr bool operator==(_sentinelWrapper<S> left, const DerivedIter & right)  { return !(right != left); }
};

} // oel
