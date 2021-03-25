#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"


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

		AssignableBox(const Type_unique_name_for_MSVC & src) noexcept
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
	static_assert(iter_is_forward<Iterator>::value, "transform_iterator requires a forward_iterator to wrap");

	using iterator_category = std::conditional_t<
			std::is_copy_constructible<UnaryFunc>::value,
			std::forward_iterator_tag,
			std::input_iterator_tag
		>;
	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( std::declval<UnaryFunc &>()(*_m.inner) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	transform_iterator() = default;
	transform_iterator(UnaryFunc f, Iterator it)  : _m{std::move(it), std::move(f)} {}

	Iterator         base() &&       OEL_ALWAYS_INLINE { return std::move(_m.inner); }
	const Iterator & base() const &  OEL_ALWAYS_INLINE { return _m.inner; }

	reference operator*() const
		OEL_REQUIRES(std::invocable< UnaryFunc const, decltype(*_m.inner) >)
	{
		return static_cast<const UnaryFunc &>(_m.func())(*_m.inner);
	}
	reference operator*()
	{
		return static_cast<UnaryFunc &>(_m.func())(*_m.inner);
	}

	transform_iterator & operator++()  OEL_ALWAYS_INLINE
	{	// pre-increment
		++_m.inner;
		return *this;
	}

	transform_iterator operator++(int) &
	{	// post-increment
		auto tmp = *this;
		++_m.inner;
		return tmp;
	}

	difference_type operator -(const transform_iterator & right) const              { return _m.inner - right._m.inner; }
	template< typename Sentinel >
	friend difference_type operator -(const transform_iterator & left, Sentinel right)  { return left._m.inner - right; }
	template< typename Sentinel >
	friend difference_type operator -(Sentinel left, const transform_iterator & right)  { return left - right._m.inner; }

	bool        operator==(const transform_iterator & right) const       { return _m.inner == right._m.inner; }
	bool        operator!=(const transform_iterator & right) const       { return _m.inner != right._m.inner; }
	template< typename Sentinel >
	friend bool operator==(const transform_iterator & left, Sentinel right)  { return left._m.inner == right; }
	template< typename Sentinel >
	friend bool operator==(Sentinel left, const transform_iterator & right)  { return right._m.inner == left; }
	template< typename Sentinel >
	friend bool operator!=(const transform_iterator & left, Sentinel right)  { return left._m.inner != right; }
	template< typename Sentinel >
	friend bool operator!=(Sentinel left, const transform_iterator & right)  { return right._m.inner != left; }
};


namespace _detail
{
	template< typename F, typename RandomAccessIter >
	auto SentinelAt(const transform_iterator<F, RandomAccessIter> & it, iter_difference_t<RandomAccessIter> n)
	->	decltype( std::true_type{iter_is_random_access<RandomAccessIter>()},
		          it.base() + n )
		 { return it.base() + n; }
}

} // namespace oel
