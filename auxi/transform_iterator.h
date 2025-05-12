#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "iterator_facade.h"
#include "transform_view.h"


namespace oel
{
namespace _detail
{
	template< bool /*CanCallConst*/, typename Func, typename Iter >
	struct TransformIterBase
	{
		using FnRef = const Func &;

		static constexpr auto can_call_const = true;

		OEL_NO_UNIQUE_ADDRESS typename AssignableWrap<Func>::Type fn;
		Iter it;
	};

	template< typename Func, typename Iter >
	struct TransformIterBase<false, Func, Iter>
	{
		using FnRef = Func &;

		static constexpr auto can_call_const = false;

		OEL_NO_UNIQUE_ADDRESS typename AssignableWrap<Func>::Type mutable fn;
		Iter it;
	};
}

////////////////////////////////////////////////////////////////////////////////

namespace iter
{

template< typename Func, typename Iterator >
class _iterTransform
 :	public _iteratorFacade< _iterTransform<Func, Iterator>, iter_difference_t<Iterator> >,
	private oel::_detail::TransformIterBase
	<	std::is_invocable_v<const Func &, const Iterator &>,
		Func, Iterator
	>
{
	using _super = typename _iterTransform::TransformIterBase;

	using _super::it;

public:
	using iterator_category = decltype( iter::_transformCategory<_super::can_call_const, Func, Iterator>() );

	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( std::declval<typename _super::FnRef>()(std::declval<const Iterator &>()) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	_iterTransform() = default;
	constexpr _iterTransform(Func f, Iterator it)   : _super{std::move(f), std::move(it)} {}

	constexpr const Iterator & base() const & noexcept   OEL_ALWAYS_INLINE { return it; }
	constexpr Iterator         base() && noexcept
		{
			static_assert(std::is_nothrow_move_constructible_v<Iterator>);
			return std::move(it);
		}

	constexpr reference operator*() const
		{
			typename _super::FnRef f = this->fn;
			return f(it);
		}

	constexpr _iterTransform & operator++()  OEL_ALWAYS_INLINE
		{
			++it;  return *this;
		}
	//! Post-increment: return type is _iterTransform if iterator_category is-a forward_iterator_tag, else void
	constexpr auto operator++(int) &
		{
			if constexpr (std::is_same_v<iterator_category, std::input_iterator_tag>)
			{
				++it;
			}
			else
			{	auto tmp = *this;
				++it;
				return tmp;
			}
		}
	constexpr _iterTransform & operator--()  OEL_ALWAYS_INLINE
		{
			--it;  return *this;
		}
	constexpr _iterTransform   operator--(int) &
		{
			auto tmp = *this;
			--it;
			return tmp;
		}

	constexpr _iterTransform & operator+=(difference_type offset) &
		{
			it += offset;
			return *this;
		}

	friend constexpr difference_type operator -(const _iterTransform & left, const _iterTransform & right)
		OEL_REQUIRES(std::sized_sentinel_for<Iterator, Iterator>)
		{
			return left.it - right.it;
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -(_sentinelWrapper<S> left, const _iterTransform & right)
		{
			return left._s - right.it;
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -(const _iterTransform & left, _sentinelWrapper<S> right)
		{
			return left.it - right._s;
		}

	friend constexpr bool operator!=
		(const _iterTransform & left, const _iterTransform & right)   { return left.it != right.it; }
	friend constexpr bool operator <
		(const _iterTransform & left, const _iterTransform & right)   { return left.it < right.it; }

	template< typename S >
	friend constexpr bool operator!=
		(const _iterTransform & left, _sentinelWrapper<S> right)   { return left.it != right._s; }
};

} // namespace iter

#if __cpp_lib_concepts < 201907
	template< typename F, typename I >
	inline constexpr bool disable_sized_sentinel_for< iter::_iterTransform<F, I>, iter::_iterTransform<F, I> >
		= disable_sized_sentinel_for<I, I>;

	template< typename S, typename F, typename I >
	inline constexpr bool disable_sized_sentinel_for< _sentinelWrapper<S>, iter::_iterTransform<F, I> >
		= disable_sized_sentinel_for<S, I>;
#endif

}