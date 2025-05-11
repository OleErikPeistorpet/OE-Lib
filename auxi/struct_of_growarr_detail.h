#pragma once

// Copyright 2024 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../view/counted.h"
#include "../view/move.h"
#include "../view/zip_transform.h"

#include <tuple>


namespace oel
{
namespace _detail
{
	constexpr auto to_tuple = [](auto &&... values)
		{
			return std::tuple{ static_cast< decltype(values) >(values)... };
		};
}


using std::align_val_t;


//! TODO doc
template< typename, typename T, align_val_t = align_val_t{} >
struct field_array  {};


/*
template< typename ElemStruct, typename ElemTag >
constexpr auto convert_field_arrays =
	[]< typename X, typename T, typename A >(field_array<X, T, A> &... fields)
	{
		return ElemStruct{ field_array {fields()}... };
	};
	*/
template< typename ElemStruct >
constexpr auto convert_field_arrays =
	[](auto &... fields)
	{
		return ElemStruct{ {fields._val}... };
	};

template< typename ElemStructL, typename ElemStructR >
auto assign_field_arrays(ElemStructL & left, const ElemStructR & right)
	{
		left._apply(_detail::to_tuple) = right._apply(_detail::to_tuple);
	}

template< typename ElemStruct >
auto swap_field_arrays(ElemStruct & a, ElemStruct & b)
	{
		a._apply(_detail::to_tuple).swap( b._apply(_detail::to_tuple) );
	}


//! TODO doc
#define OEL_STRUCT_OF_GROWARR_FIELDS(ElemStruct, ...)  \
	\
	template< typename F >       \
	auto _apply(F fn_) const     \
	{                            \
		return fn_(__VA_ARGS__); \
	}                            \
	\
	template< typename X >                                         \
	operator ElemStruct<X>()                                       \
	{                                                              \
		return _apply(oel::convert_field_arrays< ElemStruct<X> >); \
	}                                                              \
	\
	template< typename X >                         \
	void operator =(const ElemStruct<X> & other_)  \
	{                                              \
		oel::assign_field_arrays(*this, other_);   \
	}                                              \
	\
	friend void swap(ElemStruct & struct0, ElemStruct & struct1)  \
	{                                                             \
		oel::swap_field_arrays(struct0, struct1);                 \
	}



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


	template
	<	typename Ptr,
		typename Base = decltype( view::counted<Ptr>() | view::move )
	>
	struct CountedMoveView : public Base
	{
		CountedMoveView(Ptr p, ptrdiff_t n)
		 :	Base{ {}, {p, n} } {}
	};


	struct ViewTag {};
	struct ConstViewTag {};
	struct RvalueViewTag {};
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
struct field_array<_detail::RvalueViewTag, T, A>
 :	public _detail::CountedMoveView<T *>
{};


template< typename T, align_val_t A >
struct field_array<_detail::ElementTag, T, A>
{
	T & _val;

	T &       operator()() noexcept        { return _val; }
	const T & operator()() const noexcept  { return _val; }

	template< typename ElemTag >
	void operator =(field_array<ElemTag, T, A> other)
		{
			_val = other();
		}

	friend void swap(field_array & a, field_array & b)
		noexcept(std::is_nothrow_swappable<T>)
		{
			using std::swap;
			swap(a._val, b._val);
		}
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
