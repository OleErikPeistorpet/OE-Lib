#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "counted.h"
#include "../util.h" // for ssize

#if !defined(OEL_SPAN_T) and __cpp_lib_span

#include <span>

#define OEL_SPAN_T std::span
#endif

/** @file
*/

namespace oel
{
namespace view
{

struct _lastFn
{
	//! Right-hand side of operator |
	template< typename Integer >
	constexpr auto operator()(Integer count) const;

	template< typename SizedRange >
	constexpr auto operator()(SizedRange && r, range_difference_t<SizedRange> count) const
		{
			return static_cast<SizedRange &&>(r) | (*this)(count);
		}
};
inline constexpr _lastFn last;

struct _dropLastFn
{
	template< typename Integer >
	constexpr auto operator()(Integer count) const;

	template< typename SizedRange >
	constexpr auto operator()(SizedRange && r, range_difference_t<SizedRange> count) const
		{
			return static_cast<SizedRange &&>(r) | (*this)(count);
		}
};
//! Return a view consisting of all but the last count elements from the source range
/** @pre The source r must have at least count elements
* @return Type counted_view. */
inline constexpr _dropLastFn drop_last;

} // view



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename Integer >
	struct LastPartial
	{
		Integer _n;

		template< typename Range >
		friend constexpr auto operator |(Range && r, LastPartial d)
		{
			static_assert( range_is_borrowed<Range> );
			OEL_ASSERT(d._n <= oel::ssize(r));
			return view::counted(
				std::next(oel::begin_(r), oel::ssize(r) - d._n),
				d._n );
		}

	#ifdef OEL_SPAN_T
		template< typename T >
		friend constexpr auto operator |(OEL_SPAN_T<T> r, LastPartial d)
		{
			return r.last(d._n);
		}
	#endif
	};

	template< typename Integer >
	struct DropLastPartial
	{
		Integer _n;

		template< typename Range >
		friend constexpr auto operator |(Range && r, DropLastPartial d)
		{
			static_assert( range_is_borrowed<Range> );
			OEL_ASSERT(d._n <= oel::ssize(r));
			return view::counted(oel::begin_(r), oel::ssize(r) - d._n);
		}

	#ifdef OEL_SPAN_T
		template< typename T >
		friend constexpr auto operator |(OEL_SPAN_T<T> r, DropLastPartial d)
		{
			return r.first(oel::ssize(r) - d._n);
		}
	#endif
	};
}

template< typename Integer >
constexpr auto view::_lastFn::operator()(Integer n) const
{
	return _detail::LastPartial<Integer>{n};
}

template< typename Integer >
constexpr auto view::_dropLastFn::operator()(Integer n) const
{
	return _detail::DropLastPartial<Integer>{n};
}

} // oel
