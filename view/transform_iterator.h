#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"  // for TightPair
#include "../auxi/assignable.h"

#include <tuple>

/** @file
*/

namespace oel
{

using std::tuple;

/** @brief Similar to boost::transform_iterator, but supports arbitrary number of wrapped iterators
*
* And unlike boost::transform_iterator:
* - Has no size overhead for stateless function objects
* - Accepts a lambda as long as any by-value captures are trivially copy constructible and trivially destructible
* - Move-only Func and Iterator supported (then transform_iterator itself becomes move-only) */
template< bool IsZip, typename Func, typename... Iterators >
class transform_iterator
{
	using _iSeq = std::index_sequence_for<Iterators...>;

	static constexpr bool _isBidirectional{(... and iter_is_bidirectional<Iterators>)};

public:
	using iterator_category = std::conditional_t<
			std::is_copy_constructible_v<Func>,
			std::conditional_t<
				_isBidirectional,
				std::bidirectional_iterator_tag,
				std::conditional_t<
					(... and iter_is_forward<Iterators>),
					std::forward_iterator_tag,
					std::input_iterator_tag
				>
			>,
			std::input_iterator_tag
		>;
	using difference_type = std::common_type_t< iter_difference_t<Iterators>... >;
	using reference       = decltype( std::declval<const Func &>()(*std::declval<const Iterators &>()...) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	transform_iterator() = default;
	constexpr transform_iterator(Func f, Iterators... it)   : _m{ {std::move(it)...}, std::move(f) } {}

	//! Return type is `tuple<Iterators...>` if IsZip, else the underlying iterator
	constexpr auto base() &&
		{
			if constexpr (IsZip)
				return std::move(_m.first);
			else
				return std::get<0>(std::move(_m.first));
		}
	//! Return type is `const tuple<Iterators...> &` if IsZip, else the underlying iterator
	constexpr const auto & base() const & noexcept   OEL_ALWAYS_INLINE
		{
			if constexpr (IsZip)
				return _m.first;
			else
				return std::get<0>(_m.first);
		}

	constexpr reference operator*() const   OEL_ALWAYS_INLINE { return _applyDeref(_iSeq{}); }

	constexpr transform_iterator & operator++()
		{
			_increment(_iSeq{});  return *this;
		}
	//! Post-increment: return type is transform_iterator if iterator_category is-a forward_iterator_tag, else void
	constexpr auto                 operator++(int) &
		{
			if constexpr (iter_is_forward<transform_iterator>)
			{
				auto tmp = *this;
				_increment(_iSeq{});
				return tmp;
			}
			else
			{	_increment(_iSeq{});
			}
		}
	constexpr transform_iterator & operator--()
		OEL_REQUIRES(_isBidirectional)
		{
			_decrement(_iSeq{});  return *this;
		}
	constexpr transform_iterator   operator--(int) &
		OEL_REQUIRES(_isBidirectional)
		{
			auto tmp = *this;
			_decrement(_iSeq{});
			return tmp;
		}

	constexpr difference_type operator -(const transform_iterator & right) const  OEL_ALWAYS_INLINE
		OEL_REQUIRES(... and std::sized_sentinel_for<Iterators, Iterators>)
		{
			return _diff(_m.first, right._m.first, _iSeq{});
		}
	template< typename... Ss >  OEL_ALWAYS_INLINE
		OEL_REQUIRES(... and std::sized_sentinel_for<Ss, Iterators>)
	friend constexpr difference_type operator -(const transform_iterator & left, tuple<Ss...> right)
		{
			return _diff(left._m.first, right, _iSeq{});
		}
	template< typename... Ss >  OEL_ALWAYS_INLINE
		OEL_REQUIRES(... and std::sized_sentinel_for<Ss, Iterators>)
	friend constexpr difference_type operator -(tuple<Ss...> left, const transform_iterator & right)
		{
			return _diff(left, right._m.first, _iSeq{});
		}

	constexpr bool operator!=
		(const transform_iterator & right) const   OEL_ALWAYS_INLINE { return _unequal(right._m.first, _iSeq{}); }
	constexpr bool operator==
		(const transform_iterator & right) const   OEL_ALWAYS_INLINE { return !_unequal(right._m.first, _iSeq{}); }

	template< typename... Ss >  OEL_ALWAYS_INLINE
	friend constexpr bool operator!=
		(const transform_iterator & left, tuple<Ss...> right)   { return left._unequal(right, _iSeq{}); }

	template< typename... Ss >  OEL_ALWAYS_INLINE
	friend constexpr bool operator!=
		(tuple<Ss...> left, const transform_iterator & right)   { return right._unequal(left, _iSeq{}); }

	template< typename... Ss >  OEL_ALWAYS_INLINE
	friend constexpr bool operator==
		(const transform_iterator & left, tuple<Ss...> right)   { return !left._unequal(right, _iSeq{}); }

	template< typename... Ss >  OEL_ALWAYS_INLINE
	friend constexpr bool operator==
		(tuple<Ss...> left, const transform_iterator & right)   { return !right._unequal(left, _iSeq{}); }

////////////////////////////////////////////////////////////////////////////////

private:
	_detail::TightPair< tuple<Iterators...>, typename _detail::AssignableWrap<Func>::Type > _m;

	template< size_t... Ns >
	constexpr reference _applyDeref(std::index_sequence<Ns...>) const
	{
		const Func & f = _m.second();
		static_assert(std::is_same_v< reference, decltype( f(*std::get<Ns>(_m.first)...) ) >);
		return f(*std::get<Ns>(_m.first)...);
	}

	template< size_t... Ns >
	OEL_ALWAYS_INLINE constexpr void _increment(std::index_sequence<Ns...>)
	{
		(++std::get<Ns>(_m.first), ...);
	}

	template< size_t... Ns >
	OEL_ALWAYS_INLINE constexpr void _decrement(std::index_sequence<Ns...>)
	{
		(--std::get<Ns>(_m.first), ...);
	}

	template< typename... Ts, typename... Us, size_t... Ns >
	static constexpr difference_type _diff
		(const tuple<Ts...> & left, const tuple<Us...> & right, std::index_sequence<Ns...>)
	{
		return std::min({std::get<Ns>(left) - std::get<Ns>(right)...});
	}

	template< typename... Ss, size_t... Ns >
	constexpr bool _unequal(const tuple<Ss...> & sens, std::index_sequence<Ns...>) const
	{
		return (... and ( std::get<Ns>(_m.first) != std::get<Ns>(sens) ));
	}
};

#if __cpp_lib_concepts < 201907
	template< bool Z, typename F, typename... Is >
	inline constexpr bool disable_sized_sentinel_for< transform_iterator<Z, F, Is...>, transform_iterator<Z, F, Is...> >
		= (... or disable_sized_sentinel_for<Is, Is>);

	template< typename... Ss, bool Z, typename F, typename... Is >
	inline constexpr bool disable_sized_sentinel_for< tuple<Ss...>, transform_iterator<Z, F, Is...> >
		= (... or disable_sized_sentinel_for<Ss, Is>);
#endif

template< typename... Ss >
inline constexpr bool enable_unbounded_sentinel< tuple<Ss...> >
	= (... and enable_unbounded_sentinel<Ss>);

////////////////////////////////////////////////////////////////////////////////

template< typename F, typename I >
transform_iterator(F, I) -> transform_iterator<false, F, I>;

inline constexpr auto make_zip_transform_iter =
	[](auto func, auto... iterators)
	{
		return transform_iterator< true, decltype(func), decltype(iterators)... >{std::move(func), std::move(iterators)...};
	};

} // namespace oel
