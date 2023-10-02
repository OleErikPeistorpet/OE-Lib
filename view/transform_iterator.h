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
	template< typename Func, typename Iter,
		bool = std::is_invocable_v< const Func &, decltype(*std::declval<Iter const>()) >
	>
	struct TransformIterBase
	{
		using FnRef = const Func &;

		static constexpr auto canCallConst = true;

		TightPair< Iter, typename AssignableWrap<Func>::Type > m;
	};

	template< typename Func, typename Iter >
	struct TransformIterBase<Func, Iter, false>
	{
		using FnRef = Func &;

		static constexpr auto canCallConst = false;

		TightPair< Iter, typename AssignableWrap<Func>::Type > mutable m;
	};
}


/** @brief Similar to boost::transform_iterator
*
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

	using _super::m;

public:
	using iterator_category =
		std::conditional_t<
			std::is_copy_constructible_v<UnaryFunc> and _super::canCallConst,
			std::conditional_t<
				iter_is_bidirectional<Iterator>,
				std::bidirectional_iterator_tag,
				std::conditional_t< iter_is_forward<Iterator>, std::forward_iterator_tag, std::input_iterator_tag >
			>,
			std::input_iterator_tag
		>;
	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( std::declval<typename _super::FnRef>()(*m.first) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	transform_iterator() = default;
	constexpr transform_iterator(UnaryFunc f, Iterator it)   : _super{{std::move(it), std::move(f)}} {}

	OEL_ALWAYS_INLINE
	constexpr const Iterator & base() const & noexcept   { return m.first; }
	constexpr Iterator         base() && noexcept
		{
			static_assert(std::is_nothrow_move_constructible_v<Iterator>);
			return std::move(m.first);
		}

	constexpr reference operator*() const
		{
			typename _super::FnRef f = m.second();
			return f(*m.first);
		}

	OEL_ALWAYS_INLINE
	constexpr transform_iterator & operator++()   { ++m.first;  return *this; }
	//! Post-increment: return type is transform_iterator if iterator_category is-a forward_iterator_tag, else void
	constexpr auto                 operator++(int) &
		{
			if constexpr (std::is_same_v<iterator_category, std::input_iterator_tag>)
			{
				++m.first;
			}
			else
			{	auto tmp = *this;
				++m.first;
				return tmp;
			}
		}
	OEL_ALWAYS_INLINE
	constexpr transform_iterator & operator--()   { --m.first;  return *this; }

	constexpr transform_iterator   operator--(int) &
		{
			auto tmp = *this;
			--m.first;
			return tmp;
		}

	constexpr difference_type operator -(const transform_iterator & right) const
		OEL_REQUIRES(std::sized_sentinel_for<Iterator, Iterator>)        { return m.first - right.m.first; }

	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -
		(sentinel_wrapper<S> left, const transform_iterator & right)   { return left._s - right.m.first; }

	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -
		(const transform_iterator & left, sentinel_wrapper<S> right)   { return left.m.first - right._s; }

	constexpr bool operator==(const transform_iterator & right) const   { return m.first == right.m.first; }
	// These are not hidden friends because MSC 2017 gives error C3615
	constexpr bool operator!=(const transform_iterator & right) const   { return m.first != right.m.first; }

	template< typename S >
	friend constexpr bool operator==
		(const transform_iterator & left, sentinel_wrapper<S> right)   { return left.m.first == right._s; }

	template< typename S >
	friend constexpr bool operator==
		(sentinel_wrapper<S> left, const transform_iterator & right)   { return right == left; }

	template< typename S >
	friend constexpr bool operator!=
		(const transform_iterator & left, sentinel_wrapper<S> right)   { return left.m.first != right._s; }

	template< typename S >
	friend constexpr bool operator!=
		(sentinel_wrapper<S> left, const transform_iterator & right)   { return right.m.first != left._s; }
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
