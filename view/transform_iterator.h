#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail/core.h"


namespace oel
{

/** @brief Similar to boost::transform_iterator
*
* Move-only UnaryFunc and Iterator supported, but then transform_iterator
* itself becomes move-only, and oel views don't handle that. */
template< typename UnaryFunc, typename Iterator >
class transform_iterator
{
	_detail::TightPair< Iterator, typename _detail::AssignableWrap<UnaryFunc>::Type > _m;

public:
	using iterator_category = std::conditional_t<
			iter_is_forward<Iterator> and std::is_copy_constructible<UnaryFunc>::value,
			std::forward_iterator_tag,
			std::input_iterator_tag
		>;
	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( std::declval<UnaryFunc &>()(*_m.first) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	transform_iterator() = default;
	constexpr transform_iterator(UnaryFunc f, Iterator it)   : _m{std::move(it), std::move(f)} {}

	constexpr Iterator         base() &&       OEL_ALWAYS_INLINE { return std::move(_m.first); }
	constexpr const Iterator & base() const &  OEL_ALWAYS_INLINE { return _m.first; }

	constexpr reference operator*() const
		OEL_REQUIRES(std::invocable< UnaryFunc const, decltype(*_m.first) >)
		{
			const UnaryFunc & f = _m.second();
			return f(*_m.first);
		}
	constexpr reference operator*()
		{
			UnaryFunc & f = _m.second();
			return f(*_m.first);
		}

	constexpr transform_iterator & operator++()   OEL_ALWAYS_INLINE { ++_m.first;  return *this; }
	//! Post-increment: return type is transform_iterator if iterator_category is-a forward_iterator_tag, else void
	template< typename T = transform_iterator,
	          enable_if< iter_is_forward<T> > = 0
	>
	constexpr transform_iterator operator++(int) &
		{
			auto tmp = *this;
			++_m.first;
			return tmp;
		}
	template< typename T = transform_iterator,
	          enable_if< ! iter_is_forward<T> > = 0
	>	OEL_ALWAYS_INLINE
	constexpr void operator++(int) &   { ++_m.first; }

	constexpr difference_type operator -(const transform_iterator & right) const
		OEL_REQUIRES(std::sized_sentinel_for<Iterator, Iterator>)        { return _m.first - right._m.first; }

	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -(const transform_iterator & left, S right)  { return left._m.first - right; }

	template< typename S >
		OEL_REQUIRES(std::sized_sentinel_for<S, Iterator>)
	friend constexpr difference_type operator -(S left, const transform_iterator & right)  { return left - right._m.first; }

	constexpr bool        operator==(const transform_iterator & right) const       { return _m.first == right._m.first; }
	constexpr bool        operator!=(const transform_iterator & right) const       { return _m.first != right._m.first; }
	template< typename Sentinel >
	friend constexpr bool operator==(const transform_iterator & left, Sentinel right)  { return left._m.first == right; }
	template< typename Sentinel >
	friend constexpr bool operator==(Sentinel left, const transform_iterator & right)  { return right._m.first == left; }
	template< typename Sentinel >
	friend constexpr bool operator!=(const transform_iterator & left, Sentinel right)  { return left._m.first != right; }
	template< typename Sentinel >
	friend constexpr bool operator!=(Sentinel left, const transform_iterator & right)  { return right._m.first != left; }
};



////////////////////////////////////////////////////////////////////////////////



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
