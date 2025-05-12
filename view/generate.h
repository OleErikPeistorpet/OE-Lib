#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "counted.h"
#include "../auxi/detail_assignable.h"

/** @file
*/

namespace oel
{
namespace _detail
{
	template< typename G,
	          bool = std::is_invocable_v<const G &>
	>
	struct GenerateIterBase
	{
		using FnRef = const G &;

		OEL_NO_UNIQUE_ADDRESS MakeAssignable<G> f;
	};

	template< typename G >
	struct GenerateIterBase<G, false>
	{
		using FnRef = G &;

		OEL_NO_UNIQUE_ADDRESS MakeAssignable<G> mutable f;
	};
}


template< typename Generator >
class generate_iterator
 :	private _detail::GenerateIterBase<Generator>
{
	using _base = _detail::GenerateIterBase<Generator>;

public:
	using iterator_category = std::input_iterator_tag;
	using reference         = decltype( std::declval<typename _base::FnRef>()() );
	using pointer           = void;
	using value_type        = std::remove_cv_t< std::remove_reference_t<reference> >;
	using difference_type   = ptrdiff_t;

	constexpr explicit generate_iterator(Generator g)   : _base{std::move(g)} {}

	constexpr reference operator*() const
		{
			typename _base::FnRef g = this->f;  return g();
		}

	OEL_ALWAYS_INLINE
	constexpr generate_iterator & operator++()       { return *this; }
	OEL_ALWAYS_INLINE
	constexpr void                operator++(int) &  {}
};


namespace view
{

struct _generateFn
{
	//! Returns a view that generates `count` elements by calling the given generator function
	/**
	* Almost same as `generate_n` in the Range-v3 library. */
	template< typename Generator >
	constexpr auto operator()(Generator g, ptrdiff_t count) const
		{
			return counted( generate_iterator{std::move(g)}, count );
		}
#if __cpp_lib_concepts >= 201907

	//! Returns an unbounded view that generates elements by calling the given generator function
	/**
	* Almost same as `generate` in the Range-v3 library. */
	template< typename Generator >
	constexpr auto operator()(Generator g) const
		{
			return subrange( generate_iterator{std::move(g)}, std::unreachable_sentinel );
		}
#endif
};
inline constexpr _generateFn generate;

} // view

}