#pragma once

// Copyright 2024 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../view/counted.h"
#include "../view/zip_transform.h"


namespace oel
{

using std::align_val_t;


template< typename, typename T, align_val_t = align_val_t{} >
struct field_array  {};



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< bool AddConst, typename P >
	auto ptr_as_const_impl()
	{
		using PT = std::pointer_traits<P>;
		if constexpr (AddConst)
			return PT::template rebind< typename PT::element_type const >();
		else
			return P{};
	}

	template< bool AddConst, typename P >
	using PtrAsConst = decltype( ptr_as_const_impl<AddConst, P>() );


	template< bool Const, typename F >
	struct ZipTransform
	{	// TODO: optimize non-empty F sentinel
		F      fn;
		size_t count;

		template< typename... Ts >
		auto operator()(const Ts &... fields)
		{
			return view::zip_transform_n
			(	std::move(fn),
				count,
				PtrAsConst< Const, decltype(fields.p) >(fields.p)...
			);
		}
	};


	template< typename Ptr >
	struct InternalTag {};

	template< typename Ptr >
	struct ViewTag {};
	template< typename Ptr >
	struct ConstViewTag {};

	struct ElementTag {};
	struct ConstElementTag {};
	struct RvalueElementTag {};
}


template< typename Ptr, typename T, align_val_t A >
struct field_array< _detail::InternalTag<Ptr>, T, A >
{
	std::pointer_traits<Ptr>::template rebind<T> p;
};


template< typename Ptr, typename T, align_val_t A >
struct field_array< _detail::ViewTag<Ptr>, T, A >
 :	public view::counted
	<	typename std::pointer_traits<Ptr>::template rebind<T>
	> {};

template< typename Ptr, typename T, align_val_t A >
struct field_array< _detail::ConstViewTag<Ptr>, T, A >
 :	public view::counted
	<	typename std::pointer_traits<Ptr>::template rebind<T const>
	> {};


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

template< typename T, align_val_t A >
struct field_array<_detail::RvalueElementTag, T, A>
{
	T && _val;

	T && operator()() noexcept         { return std::move(_val); }

	explicit operator T &&() noexcept  { return std::move(_val); }
};

} // oel
