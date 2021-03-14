#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../counted.h"
#include "../subrange.h"
#include "../transform_iterator.h"

namespace oel
{
namespace _detail
{
	template< typename ViewIter, typename Range, typename... None >
	constexpr basic_view<ViewIter> All(Range & r, None...)
	{
		return{ ViewIter{begin(r)}, ViewIter{end(r)} };
	}

	template< typename ViewIter, typename SizedRange >
	constexpr auto All(SizedRange & r)
	->	decltype(counted_view<ViewIter>( ViewIter{begin(r)}, oel::ssize(r) ))
	{	return   counted_view<ViewIter>( ViewIter{begin(r)}, oel::ssize(r) ); }



	template< typename Func, typename Iterator >
	struct Transf
	{
		using TI = transform_iterator<Func, Iterator>;

		template< typename F_ = Func,
		          enable_if< std::is_empty<F_>::value > = 0
		>
		static constexpr TI _sentinel(Func f, Iterator last)
		{
			return {f, last};
		}

		template< typename... None >
		static constexpr Iterator _sentinel(Func, Iterator last, None...)
		{
			return last;
		}


		static constexpr auto call(basic_view<Iterator> v, Func f)
		{
			return view::subrange( TI{f, v.begin()}, _sentinel(f, v.end()) );
		}

		static constexpr auto call(counted_view<Iterator> v, Func f)
		{
			return counted_view<TI>( {f, v.begin()}, v.size() );
		}
	};
}

}