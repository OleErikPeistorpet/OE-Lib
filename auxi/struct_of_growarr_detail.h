#pragma once

// Copyright 2024 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../view/counted.h"
#include "../view/move.h"
#include "../view/zip_transform.h"

#include <tuple>


//! TODO doc
#define OEL_STRUCT_OF_GROWARR_FIELDS(elem_struct, ...)  \
	\
	template< typename F >                      \
	auto _apply(F fn_) const                    \
		noexcept(noexcept( fn_(__VA_ARGS__) ))  \
		{        return    fn_(__VA_ARGS__); }  \
	\
	template< typename ElemTag >                          \
	operator elem_struct<ElemTag>() const                 \
	{                                                     \
		return oel::convert_field_arrays<ElemTag>(*this); \
	}                                                     \
	\
	template< typename ElemTag >                                   \
	elem_struct & operator =(const elem_struct<ElemTag> & other_)  \
	{                                                              \
		oel::assign_field_arrays(*this, other_);                   \
		return *this;                                              \
	}                                                              \
	\
	template< typename ElemTag >                                \
	void operator =(const elem_struct<ElemTag> & other_) const  \
	{                                                           \
		oel::assign_field_arrays(*this, other_);                \
	}                                                           \
	\
	friend void swap(elem_struct & struct0, elem_struct & struct1)      \
		noexcept(noexcept( oel::swap_field_arrays(struct0, struct1) ))  \
		{                  oel::swap_field_arrays(struct0, struct1); }


namespace oel
{

using std::align_val_t;


//! TODO doc
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


	template< typename T >
	T & as_lvalue(T && ob) noexcept { return ob; }


	struct ViewTag {};
	struct ConstViewTag {};
	struct RvalueViewTag {};
	struct ElementTag {};
	struct ConstElementTag {};
	struct RvalueElementTag {};
	struct InternalTag {};


	template
	<	typename Ptr,
		typename Base = decltype( view::counted<Ptr>() | view::move )
	>
	struct CountedMoveView : public Base
	{
		CountedMoveView(Ptr p, ptrdiff_t n)
		 :	Base{ {}, {p, n} } {}
	};


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


	inline constexpr auto fields_to_tuple =
		[](auto &... fields) noexcept
		{
			return std::tuple{fields...};
		};

	template< typename Tuple0, typename Tuple1, size_t... Ns >
	void assign_tuple_ref(Tuple0 & dest, const Tuple1 & src, std::index_sequence<Ns...>)
	{
		(( std::get<Ns>(dest) = std::get<Ns>(src) ), ...);
	}
}


template
<	typename ElemTagTo,
	template< typename > typename ElemStruct,
	typename ElemTagFrom
>
auto convert_field_arrays(const ElemStruct<ElemTagFrom> & s)
	{
		return s._apply(
			[](auto &... fields)
			{
				return ElemStruct<ElemTagTo>{fields._val...};
			} );
	}

template< typename ElemStructL, typename ElemStructR >
void assign_field_arrays(ElemStructL & left, const ElemStructR & right)
	{
		auto l = left._apply(_detail::fields_to_tuple);
		auto r = right._apply(_detail::fields_to_tuple);
		_detail::assign_tuple_ref(
			l, r,
			std::make_index_sequence< std::tuple_size_v<decltype(l)> >() );
	}

template< typename ElemStruct >
void swap_field_arrays(ElemStruct & a, ElemStruct & b)
	noexcept(noexcept( a._apply(_detail::fields_to_tuple).swap(_detail::as_lvalue( b._apply(_detail::fields_to_tuple) )) ))
	{                  a._apply(_detail::fields_to_tuple).swap(_detail::as_lvalue( b._apply(_detail::fields_to_tuple) )); }


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

	void operator =(field_array other) const
		{
			_val = other._val;
		}

	template< typename ElemTag2 >
	void operator =(field_array<ElemTag2, T, A> other) const
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
	T & _val;

	T && operator()() noexcept  { return std::move(_val); }

	field_array(T && val) noexcept  : _val{val} {}
};


template< typename T, align_val_t A >
struct field_array<_detail::InternalTag, T, A>
	{
		T * p;
	};

} // oel


template
<	template< typename > typename ElemStruct,
	template< typename > typename T,
	template< typename > typename U
>
struct std::basic_common_reference
<	ElemStruct<oel::_detail::ElementTag>,
	ElemStruct<oel::_detail::RvalueElementTag>,
	T, U
>
{	using type = ElemStruct<oel::_detail::ConstElementTag>;
};

template
<	template< typename > typename ElemStruct,
	template< typename > typename T,
	template< typename > typename U
>
struct std::basic_common_reference
<	ElemStruct<oel::_detail::RvalueElementTag>,
	ElemStruct<oel::_detail::ElementTag>,
	T, U
>
{	using type = ElemStruct<oel::_detail::ConstElementTag>;
};
