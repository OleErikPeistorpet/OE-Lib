#pragma once

// Copyright 2024 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../view/counted.h"


namespace oel
{

using std::align_val_t;


template< typename, typename T, align_val_t = align_val_t{} >
struct field_array  {};



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< bool AddConst, typename P >
	auto ptr_as_const(P ptr) noexcept
	{
		using PT = std::pointer_traits<P>;
		if constexpr (AddConst)
			return PT::template rebind< typename PT::element_type const >(ptr);
		else
			return ptr;
	}


	template< typename RefStruct >
	struct Zip
	{
		template< typename... Ts >
		RefStruct operator()(Ts &&... vals) const noexcept
		{
			return{ static_cast<Ts &&>(vals)... };
		}
	};


	struct InternalTag {};
	struct ViewTag {};
	struct ConstViewTag {};
	struct ElementTag {};
	struct ConstElementTag {};
}


template< typename T, align_val_t A >
struct field_array<_detail::InternalTag, T, A>
{
	T * p;
};


template< typename T, align_val_t A >
struct field_array<_detail::ViewTag, T, A>
 :	public view::counted<T *> {};

template< typename T, align_val_t A >
struct field_array<_detail::ConstViewTag, T, A>
 :	public view::counted<const T *> {};


template< typename T, align_val_t A >
struct field_array<_detail::ElementTag, T, A>
{
	T & _val;

	T &       operator()() noexcept        { return _val; }
	const T & operator()() const noexcept  { return _val; }

	explicit operator T &() noexcept       { return _val; }
};

template< typename T, align_val_t A >
struct field_array<_detail::ConstElementTag, T, A>
{
	const T & _val;

	const T & operator()() const noexcept         { return _val; }

	explicit operator const T &() const noexcept  { return _val; }
};

} // oel
