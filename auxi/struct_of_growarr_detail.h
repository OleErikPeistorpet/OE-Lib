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
	template< bool AddConst, typename T >
	auto ptr_as_const_impl(T *)
	{
		if constexpr( AddConst )
		{
			using P = const T *;
			return P{};
		}
		else
		{	using P = T *;
			return P{};
		}
	}

	template< bool AddConst, typename P >
	using PtrAsConst = decltype( ptr_as_const_impl<AddConst>(P{}) );


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

	template< typename RefStruct, typename RvalueStruct >
	struct Zip
	{
		template< typename... Ts >
		RefStruct operator()(Ts &&... vals) const noexcept
		{
			return{ static_cast<Ts &&>(vals)... };
		}

		template< typename... Is, size_t... Ns >
		static RvalueStruct do_iter_move(const std::tuple<Is...> & iters, std::index_sequence<Ns...>) noexcept
		{
			return{ oel::iter_move( std::get<Ns>(iters) )... };
		}
	};


	struct ViewTag {};
	struct ConstViewTag {};
	struct ElementTag {};
	struct ConstElementTag {};
	struct RvalueElementTag {};
	struct InternalTag {};
}


namespace iter
{
template< typename S, typename R, typename... Iterators >
auto iter_move(const _zipTransform< oel::_detail::Zip<S, R>, Iterators... > & it) noexcept
	{
		return oel::_detail::Zip<S, R>::do_iter_move(it.base(), std::index_sequence_for<Iterators...>{});
	}
}


template< typename T, align_val_t A >
struct field_array<_detail::ViewTag, T, A>
 :	public view::counted<T *>
{};

template< typename T, align_val_t A >
struct field_array<_detail::ConstViewTag, T, A>
 :	public view::counted<const T *>
{};


template< typename T, align_val_t A >
struct field_array<_detail::ElementTag, T, A>
{
	T & _val;

	T &       operator()() noexcept        { return _val; }
	const T & operator()() const noexcept  { return _val; }
};

template< typename T, align_val_t A >
struct field_array<_detail::ConstElementTag, T, A>
{
	const T & _val;

	const T & operator()() const noexcept  { return _val; }
};

template< typename T, align_val_t A >
struct field_array<_detail::RvalueElementTag, T, A>
{
	T && _val;

	T && operator()() noexcept  { return std::move(_val); }
};


template< typename T, align_val_t A >
struct field_array<_detail::InternalTag, T, A>
	{
		T * p;
	};

} // oel
