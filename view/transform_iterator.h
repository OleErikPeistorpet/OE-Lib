#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"  // for TightPair
#include "../auxi/detail_assignable.h"

/** @file
*/

namespace oel
{
namespace _detail
{
	template
	<	typename Func, typename Iter,
		bool = std::is_invocable_v< const Func &, decltype( *std::declval<Iter const>() ) >
	>
	struct TransformIterBase
	{
		using FnRef = const Func &;

		static constexpr auto canCallConst = true;

		OEL_NO_UNIQUE_ADDRESS MakeAssignable<Func> fn;
		Iter it;
	};

	template< typename Func, typename Iter >
	struct TransformIterBase<Func, Iter, false>
	{
		using FnRef = Func &;

		static constexpr auto canCallConst = false;

		OEL_NO_UNIQUE_ADDRESS MakeAssignable<Func> mutable fn;
		Iter it;
	};
}


//! Similar to boost::transform_iterator
/**
* But unlike boost::transform_iterator:
* - Has no size overhead for stateless function objects
* - Accepts a lambda as long as any by-value captures are trivially copy constructible and trivially destructible
* - Move-only UnaryFunc and Iterator supported (then transform_iterator itself becomes move-only)
* - Function objects (including lambda) can have non-const `operator()`, then merely std::input_iterator is modeled  */
template< typename UnaryFunc, typename Iterator >
class transform_iterator
 :	private _detail::TransformIterBase<UnaryFunc, Iterator>
{
	using _super = typename transform_iterator::TransformIterBase;
	using _super::it;

	static constexpr auto _cat()
		{
			if constexpr( std::is_copy_constructible_v<UnaryFunc> and _super::canCallConst )
			{
				if constexpr( iter_is_random_access<Iterator> )
					return std::random_access_iterator_tag{};
				else
					return _detail::CheckedIterTag<Iterator>();
			}
			else
			{	return std::input_iterator_tag{};
			}
		}

public:
	using iterator_category = decltype( _cat() );

	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( std::declval<typename _super::FnRef>()(*it) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	transform_iterator() = default;
	constexpr transform_iterator(UnaryFunc f, Iterator it)   : _super{std::move(f), std::move(it)} {}

	OEL_ALWAYS_INLINE
	constexpr const Iterator & base() const & noexcept   { return it; }
	constexpr Iterator         base() && noexcept
		{
			static_assert( std::is_nothrow_move_constructible_v<Iterator> );
			return std::move(it);
		}

	constexpr reference operator*() const
		{
			typename _super::FnRef f = this->fn;
			return f(*it);
		}

	constexpr reference operator[](difference_type offset) const
		{
			const UnaryFunc & f = this->fn;
			return f(it[offset]);
		}

	OEL_ALWAYS_INLINE
	constexpr transform_iterator & operator++()   { ++it;  return *this; }
	//! Post-increment: return type is transform_iterator if iterator_category is-a forward_iterator_tag, else void
	constexpr auto                 operator++(int) &
		{
			if constexpr( std::is_same_v<iterator_category, std::input_iterator_tag> )
			{
				++it;
			}
			else
			{	auto tmp = *this;
				++it;
				return tmp;
			}
		}
	OEL_ALWAYS_INLINE
	constexpr transform_iterator & operator--()   { --it;  return *this; }

	constexpr transform_iterator   operator--(int) &
		{
			auto tmp = *this;
			--it;
			return tmp;
		}

	constexpr transform_iterator & operator+=(difference_type offset) &
		{
			it += offset;
			return *this;
		}
	constexpr transform_iterator & operator-=(difference_type offset) &
		{
			it -= offset;
			return *this;
		}

	friend constexpr transform_iterator operator +(difference_type offset, transform_iterator ti)  { return ti += offset; }
	OEL_ALWAYS_INLINE
	friend constexpr transform_iterator operator +(transform_iterator ti, difference_type offset)  { return ti += offset; }

	friend constexpr transform_iterator operator -(transform_iterator ti, difference_type offset)  { return ti -= offset; }

	friend constexpr difference_type operator -(const transform_iterator & left, const transform_iterator & right)
		OEL_REQUIRES(std::sized_sentinel_for<Iterator, Iterator>)
		{
			return left.it - right.it;
		}

	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -
		(sentinel_wrapper<S> left, const transform_iterator & right)   { return left.se - right.it; }

	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -
		(const transform_iterator & left, sentinel_wrapper<S> right)   { return left.it - right.se; }

	friend constexpr bool operator==
		(const transform_iterator & left, const transform_iterator & right)   { return left.it == right.it; }

	friend constexpr bool operator!=
		(const transform_iterator & left, const transform_iterator & right)   { return left.it != right.it; }

	friend constexpr bool operator <
		(const transform_iterator & left, const transform_iterator & right)   { return left.it < right.it; }

	friend constexpr bool operator >
		(const transform_iterator & left, const transform_iterator & right)   { return left.it > right.it; }

	friend constexpr bool operator<=
		(const transform_iterator & left, const transform_iterator & right)   { return left.it <= right.it; }

	friend constexpr bool operator>=
		(const transform_iterator & left, const transform_iterator & right)   { return left.it >= right.it; }

	template< typename S >
	friend constexpr bool operator==
		(const transform_iterator & left, sentinel_wrapper<S> right)   { return left.it == right.se; }

	template< typename S >
	friend constexpr bool operator==
		(sentinel_wrapper<S> left, const transform_iterator & right)   { return right.it == left.se; }

	template< typename S >
	friend constexpr bool operator!=
		(const transform_iterator & left, sentinel_wrapper<S> right)   { return left.it != right.se; }

	template< typename S >
	friend constexpr bool operator!=
		(sentinel_wrapper<S> left, const transform_iterator & right)   { return right.it != left.se; }
};

#if __cpp_lib_concepts < 201907
	template< typename F, typename I >
	inline constexpr bool disable_sized_sentinel_for< transform_iterator<F, I>, transform_iterator<F, I> >
		= disable_sized_sentinel_for<I, I>;

	template< typename S, typename F, typename I >
	inline constexpr bool disable_sized_sentinel_for< sentinel_wrapper<S>, transform_iterator<F, I> >
		= disable_sized_sentinel_for<S, I>;
#endif

} // namespace oel
