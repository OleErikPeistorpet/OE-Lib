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
	template< typename G, bool = std::is_invocable_v<const G &> >
	struct GenerateIterBase
	{
		using FnRef = const G &;

		typename AssignableWrap<G>::Type f;
	};

	template< typename G >
	struct GenerateIterBase<G, false>
	{
		using FnRef = G &;

		typename AssignableWrap<G>::Type mutable f;
	};
}


template< typename Generator >
class generate_iterator
 :	private _detail::GenerateIterBase<Generator>
{
	using _base = typename generate_iterator::GenerateIterBase;

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

	constexpr generate_iterator & operator++()        OEL_ALWAYS_INLINE { return *this; }
	constexpr void                operator++(int) &   OEL_ALWAYS_INLINE {}
};


namespace view
{
//! Returns a view that generates `count` elements by calling the given generator function
/**
* Like `generate_n` in the Range-v3 library, but this is only for use within OE-Lib. */
inline constexpr auto generate =
	[](auto generator, ptrdiff_t count)  { return counted(generate_iterator{std::move(generator)}, count); };
}

} // oel
