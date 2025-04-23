#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail_assignable.h"
#include "iterator_facade.h"
#include "../util.h"  // for TightPair


namespace oel
{
namespace _detail
{
	template
	<	typename Func, typename Iter,
		bool = std::is_invocable_v<const Func &, const Iter &>
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

////////////////////////////////////////////////////////////////////////////////

namespace iter
{

template< typename Func, typename Iterator >
class _iterTransform
 :	public _iteratorFacade< _iterTransform<Func, Iterator>, iter_difference_t<Iterator> >,
	private oel::_detail::TransformIterBase<Func, Iterator>
{
	using _super = typename _iterTransform::TransformIterBase;

	using _super::m;

	static constexpr auto _cat()
		{
			if constexpr( std::is_copy_constructible_v<Func> and _super::canCallConst )
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
	using reference       = decltype( std::declval<typename _super::FnRef>()(std::declval<const Iterator &>()) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	_iterTransform() = default;
	constexpr _iterTransform(Func f, Iterator it)   : _super{{ std::move(it), std::move(f) }} {}

	constexpr const Iterator & base() const & noexcept   OEL_ALWAYS_INLINE { return m.first; }
	constexpr Iterator         base() && noexcept
		{
			static_assert( std::is_nothrow_move_constructible_v<Iterator> );
			return std::move(m.first);
		}

	constexpr reference operator*() const
		{
			const Iterator & it{m.first};  // m maybe mutable, not giving f mutable access
			typename _super::FnRef f = m.second();
			return f(it);
		}

	constexpr _iterTransform & operator++()   OEL_ALWAYS_INLINE { ++m.first;  return *this; }

	//! Post-increment: return type is _iterTransform if iterator_category is-a forward_iterator_tag, else void
	constexpr auto operator++(int) &
		{
			if constexpr( std::is_same_v<iterator_category, std::input_iterator_tag> )
			{
				++m.first;
			}
			else
			{	auto tmp = *this;
				++m.first;
				return tmp;
			}
		}
	constexpr _iterTransform & operator--()   OEL_ALWAYS_INLINE { --m.first;  return *this; }

	constexpr _iterTransform   operator--(int) &
		{
			auto tmp = *this;
			--m.first;
			return tmp;
		}

	constexpr _iterTransform & operator+=(difference_type offset) &
		{
			m.first += offset;
			return *this;
		}

	friend constexpr difference_type operator -(const _iterTransform & left, const _iterTransform & right)
		OEL_REQUIRES(std::sized_sentinel_for<Iterator, Iterator>)
		{
			return left.m.first - right.m.first;
		}

	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -(_sentinelWrapper<S> left, const _iterTransform & right)
		{
			return left.se - right.m.first;
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -(const _iterTransform & left, _sentinelWrapper<S> right)
		{
			return left.m.first - right.se;
		}

	friend constexpr bool operator!=
		(const _iterTransform & left, const _iterTransform & right)   { return left.m.first != right.m.first; }

	friend constexpr bool operator <
		(const _iterTransform & left, const _iterTransform & right)   { return left.m.first < right.m.first; }

	template< typename S >
	friend constexpr bool operator!=
		(const _iterTransform & left, _sentinelWrapper<S> right)   { return left.m.first != right.se; }
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