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

struct _firstFn
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
inline constexpr _firstFn first;

struct _dropFn
{
	template< typename Integer >
	constexpr auto operator()(Integer count) const;

	template< typename SizedRange >
	constexpr auto operator()(SizedRange && r, range_difference_t<SizedRange> count) const
		{
			return static_cast<SizedRange &&>(r) | (*this)(count);
		}
};
//! Return a view consisting of all but the first count elements from the source range
/** @pre The source r must have at least count elements
* @return Type counted_view. */
inline constexpr _dropFn drop_first;

} // view



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename Integer >
	struct FirstPartial
	{
		Integer _n;

		template< typename Range >
		friend constexpr auto operator |(Range && r, FirstPartial d)
		{
			static_assert( range_is_borrowed<Range> );
			OEL_ASSERT(d._n <= oel::ssize(r));
			return view::counted(oel::begin_(r), d._n);
		}

	#ifdef OEL_SPAN_T
		template< typename T >
		friend constexpr auto operator |(OEL_SPAN_T<T> r, FirstPartial d)
		{
			return r.first(d._n);
		}
	#endif
	};

	template< typename Integer >
	struct DropPartial
	{
		Integer _n;

		template< typename Range >
		friend constexpr auto operator |(Range && r, DropPartial d)
		{
			static_assert( range_is_borrowed<Range> );
			OEL_ASSERT(d._n <= oel::ssize(r));
			return view::counted(
				std::next(oel::begin_(r), d._n),
				oel::ssize(r) - d._n );
		}

	#ifdef OEL_SPAN_T
		template< typename T >
		friend constexpr auto operator |(OEL_SPAN_T<T> r, DropPartial d)
		{
			return r.subspan(d._n);
		}
	#endif
	};
}

template< typename Integer >
constexpr auto view::_firstFn::operator()(Integer n) const
{
	return _detail::FirstPartial<Integer>{n};
}

template< typename Integer >
constexpr auto view::_dropFn::operator()(Integer n) const
{
	return _detail::DropPartial<Integer>{n};
}

} // oel
