#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail_assignable.h"
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

template< typename Func, typename Iterator >
class _iterTransformIterator
 :	private _detail::TransformIterBase<Func, Iterator>
{
	using _super = typename _iterTransformIterator::TransformIterBase;

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

	_iterTransformIterator() = default;
	constexpr _iterTransformIterator(Func f, Iterator it)   : _super{{ std::move(it), std::move(f) }} {}

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

	constexpr reference operator[](difference_type offset) const
		{
			const Func & f = m.second();
			return f(m.first + offset);
		}

	constexpr _iterTransformIterator & operator++()  OEL_ALWAYS_INLINE
		{
			++m.first;  return *this;
		}
	//! Post-increment: return type is _iterTransformIterator if iterator_category is-a forward_iterator_tag, else void
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
	constexpr _iterTransformIterator & operator--()  OEL_ALWAYS_INLINE
		{
			--m.first;  return *this;
		}
	constexpr _iterTransformIterator   operator--(int) &
		{
			auto tmp = *this;
			--m.first;
			return tmp;
		}

	constexpr _iterTransformIterator & operator+=(difference_type offset) &
		{
			m.first += offset;
			return *this;
		}
	constexpr _iterTransformIterator & operator-=(difference_type offset) &
		{
			m.first -= offset;
			return *this;
		}

	friend constexpr _iterTransformIterator operator +(_iterTransformIterator it, difference_type offset)
		{
			it.m.first += offset;
			return it;
		}
	friend constexpr _iterTransformIterator operator +
		(difference_type offset, _iterTransformIterator it)   { return it += offset; }

	friend constexpr _iterTransformIterator operator -
		(_iterTransformIterator it, difference_type offset)   { return it -= offset; }

	friend constexpr difference_type operator -(const _iterTransformIterator & left, const _iterTransformIterator & right)
		OEL_REQUIRES(std::sized_sentinel_for<Iterator, Iterator>)
		{
			return left.m.first - right.m.first;
		}

	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -(_sentinelWrapper<S> left, const _iterTransformIterator & right)
		{
			return left.se - right.m.first;
		}
	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -(const _iterTransformIterator & left, _sentinelWrapper<S> right)
		{
			return left.m.first - right.se;
		}

	friend constexpr bool operator!=(const _iterTransformIterator & left, const _iterTransformIterator & right)
		{
			return left.m.first != right.m.first;
		}
	friend constexpr bool operator==(const _iterTransformIterator & left, const _iterTransformIterator & right)
		{
			return left.m.first == right.m.first;
		}
	friend constexpr bool operator <(const _iterTransformIterator & left, const _iterTransformIterator & right)
		{
			return left.m.first < right.m.first;
		}
	friend constexpr bool operator >
		(const _iterTransformIterator & left, const _iterTransformIterator & right)  { return right < left; }

	friend constexpr bool operator<=
		(const _iterTransformIterator & left, const _iterTransformIterator & right)  { return !(right < left); }

	friend constexpr bool operator>=
		(const _iterTransformIterator & left, const _iterTransformIterator & right)  { return !(left < right); }

	template< typename S >
	friend constexpr bool operator!=
		(const _iterTransformIterator & left, _sentinelWrapper<S> right)   { return left.m.first != right.se; }

	template< typename S >
	friend constexpr bool operator!=
		(_sentinelWrapper<S> left, const _iterTransformIterator & right)   { return right.m.first != left.se; }

	template< typename S >
	friend constexpr bool operator==
		(const _iterTransformIterator & left, _sentinelWrapper<S> right)   { return left.m.first == right.se; }

	template< typename S >
	friend constexpr bool operator==
		(_sentinelWrapper<S> left, const _iterTransformIterator & right)   { return right == left; }
};

#if __cpp_lib_concepts < 201907
	template< typename F, typename I >
	inline constexpr bool disable_sized_sentinel_for< _iterTransformIterator<F, I>, _iterTransformIterator<F, I> >
		= disable_sized_sentinel_for<I, I>;

	template< typename S, typename F, typename I >
	inline constexpr bool disable_sized_sentinel_for< _sentinelWrapper<S>, _iterTransformIterator<F, I> >
		= disable_sized_sentinel_for<S, I>;
#endif

} // namespace oel
