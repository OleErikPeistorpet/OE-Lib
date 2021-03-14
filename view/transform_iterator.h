#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../auxi/type_traits.h"


namespace oel
{
namespace _detail
{
	template< typename T, typename U,
	          bool = std::is_empty<U>::value >
	struct TightPair
	{
		T inner;
		U _sec;

		OEL_ALWAYS_INLINE constexpr const U & func() const { return _sec; }
		OEL_ALWAYS_INLINE constexpr       U & func()       { return _sec; }
	};

	template< typename Type_unique_name_for_MSVC, typename Empty_type_MSVC_unique_name >
	struct TightPair<Type_unique_name_for_MSVC, Empty_type_MSVC_unique_name, true>
	 :	Empty_type_MSVC_unique_name
	{
		Type_unique_name_for_MSVC inner;

		TightPair() = default;
		constexpr TightPair(Type_unique_name_for_MSVC i, Empty_type_MSVC_unique_name f)
		 :	Empty_type_MSVC_unique_name{f}, inner{std::move(i)}
		{}

		OEL_ALWAYS_INLINE constexpr const Empty_type_MSVC_unique_name & func() const { return *this; }
		OEL_ALWAYS_INLINE constexpr       Empty_type_MSVC_unique_name & func()       { return *this; }
	};


	template< typename Type_unique_name_for_MSVC >
	struct AssignableBox : Type_unique_name_for_MSVC
	{
		static_assert(
			std::is_nothrow_copy_constructible<Type_unique_name_for_MSVC>::value and
				std::is_trivially_destructible<Type_unique_name_for_MSVC>::value,
			"transform_iterator requires move assignable or trivially destructible UnaryFunc");

		constexpr AssignableBox(const Type_unique_name_for_MSVC & src) noexcept
		 :	Type_unique_name_for_MSVC(src) {}

		AssignableBox() = default;
		AssignableBox(const AssignableBox &) = default;

		void operator =(const AssignableBox & other) & noexcept
		{
			::new(this) AssignableBox(other);
		}
	};

	template< typename T >
	using MakeAssignable = std::conditional_t< std::is_move_assignable<T>::value, T, AssignableBox<T> >;
}


/** @brief Similar to boost::transform_iterator
*
* Move-only UnaryFunc and Iterator supported, but then transform_iterator
* itself becomes move-only, and oel views don't handle that. */
template< typename UnaryFunc, typename Iterator >
class transform_iterator
{
	_detail::TightPair< Iterator, _detail::MakeAssignable<UnaryFunc> > _m;

public:
	using iterator_category = std::conditional_t<
			iter_is_forward<Iterator>::value and std::is_copy_constructible<UnaryFunc>::value,
			std::forward_iterator_tag,
			std::input_iterator_tag
		>;
	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( std::declval<UnaryFunc &>()(*_m.inner) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	transform_iterator() = default;
	constexpr transform_iterator(UnaryFunc f, Iterator it)  : _m{std::move(it), std::move(f)} {}

	constexpr Iterator         base() &&       OEL_ALWAYS_INLINE { return std::move(_m.inner); }
	constexpr const Iterator & base() const &  OEL_ALWAYS_INLINE { return _m.inner; }

	constexpr reference operator*() const
		OEL_REQUIRES(std::invocable< UnaryFunc const, decltype(*_m.inner) >)
		{
			return static_cast<const UnaryFunc &>(_m.func())(*_m.inner);
		}
	constexpr reference operator*()
		{
			return static_cast<UnaryFunc &>(_m.func())(*_m.inner);
		}

	constexpr transform_iterator & operator++()  OEL_ALWAYS_INLINE
		{
			++_m.inner;
			return *this;
		}
	//! Return type is transform_iterator if iterator_category is forward_iterator_tag, else void
	constexpr auto operator++(int) &   OEL_ALWAYS_INLINE { return _postIncrement<iterator_category>(); }

	constexpr difference_type operator -(const transform_iterator & right) const  { return _m.inner - right._m.inner; }
	template< typename Sentinel >
	friend constexpr difference_type
		operator -(const transform_iterator & left, Sentinel right)   { return left._m.inner - right; }
	template< typename Sentinel >
	friend constexpr difference_type
		operator -(Sentinel left, const transform_iterator & right)   { return left - right._m.inner; }

	constexpr bool        operator==(const transform_iterator & right) const       { return _m.inner == right._m.inner; }
	constexpr bool        operator!=(const transform_iterator & right) const       { return _m.inner != right._m.inner; }
	template< typename Sentinel >
	friend constexpr bool operator==(const transform_iterator & left, Sentinel right)  { return left._m.inner == right; }
	template< typename Sentinel >
	friend constexpr bool operator==(Sentinel left, const transform_iterator & right)  { return right._m.inner == left; }
	template< typename Sentinel >
	friend constexpr bool operator!=(const transform_iterator & left, Sentinel right)  { return left._m.inner != right; }
	template< typename Sentinel >
	friend constexpr bool operator!=(Sentinel left, const transform_iterator & right)  { return right._m.inner != left; }



////////////////////////////////////////////////////////////////////////////////
//
// Just implementation


private:
	template< typename IterCat,
		enable_if< std::is_base_of<std::forward_iterator_tag, IterCat>::value > = 0
	>
	constexpr transform_iterator _postIncrement()
	{
		auto tmp = *this;
		++_m.inner;
		return tmp;
	}

	template< typename, typename... None >
	constexpr void _postIncrement(None...)
	{
		++_m.inner;
	}
};


namespace _detail
{
	template< typename F, typename Iterator >
	struct SentinelAt< transform_iterator<F, Iterator> >
	{
		template< typename I = Iterator >
		static constexpr auto call(const transform_iterator<F, I> & it, iter_difference_t<I> n)
		->	decltype( _detail::SentinelAt<I>::call(it.base(), n) )
		{	return    _detail::SentinelAt<I>::call(it.base(), n); }
	};
}

} // namespace oel
