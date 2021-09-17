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
		T iter;
		U _sec;

		OEL_ALWAYS_INLINE const U & second() const noexcept { return _sec; }
		OEL_ALWAYS_INLINE       U & second() noexcept       { return _sec; }
	};

	template< typename Type0_unique_name_for_MSVC, typename Empty_type_MSVC_unique_name >
	struct TightPair< Type0_unique_name_for_MSVC, Empty_type_MSVC_unique_name, true >
	 :	Empty_type_MSVC_unique_name
	{
		Type0_unique_name_for_MSVC iter;

		TightPair() = default;
		TightPair(Type0_unique_name_for_MSVC i, Empty_type_MSVC_unique_name s)
		 :	Empty_type_MSVC_unique_name(s), iter(std::move(i)) {
		}

		OEL_ALWAYS_INLINE const Empty_type_MSVC_unique_name & second() const noexcept { return *this; }
		OEL_ALWAYS_INLINE       Empty_type_MSVC_unique_name & second() noexcept       { return *this; }
	};


	template< typename T,
	          bool = std::is_move_assignable<T>::value >
	class AssignableWrap
	{
		static_assert( std::is_trivially_copy_constructible<T>::value and std::is_trivially_destructible<T>::value,
			"UnaryFunc must be move assignable, or trivially copy constructible and trivially destructible" );

		union Impl
		{
			struct {} _none;
			T _val;

			Impl(const T & src) noexcept : _val(src) {}

			Impl() = default;
			Impl(const Impl &) = default;

			void operator =(const Impl & other) noexcept
			{
				::new(this) Impl(other);
			}

			OEL_ALWAYS_INLINE explicit operator const T &() const noexcept { return _val; }
			OEL_ALWAYS_INLINE explicit operator       T &() noexcept       { return _val; }
		};

		struct ImplEmpty : public T
		{
			ImplEmpty(const T & src) noexcept : T(src) {}

			ImplEmpty() = default;
			ImplEmpty(const ImplEmpty &) = default;

			void operator =(const ImplEmpty &) noexcept {}
		};

	public:
		using Type = std::conditional_t< std::is_empty<T>::value, ImplEmpty, Impl >;
	};

	template< typename T >
	class AssignableWrap<T, true>
	{
	public:
		using Type = T;
	};
}


/** @brief Similar to boost::transform_iterator
*
* Move-only UnaryFunc and Iterator supported, but then transform_iterator
* itself becomes move-only, and oel views don't handle that. */
template< typename UnaryFunc, typename Iterator >
class transform_iterator
{
	_detail::TightPair< Iterator, typename _detail::AssignableWrap<UnaryFunc>::Type > _m;

public:
	using iterator_category =
		std::conditional_t< !std::is_copy_constructible<UnaryFunc>::value,
			std::input_iterator_tag,
			std::conditional_t< iter_is_random_access<Iterator>::value,
				std::random_access_iterator_tag,
				iter_category<Iterator>
			>
		>;
	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( std::declval<UnaryFunc &>()(*_m.iter) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	transform_iterator() = default;
	transform_iterator(UnaryFunc f, Iterator it)  : _m{std::move(it), std::move(f)} {}

	const Iterator & base() const &  { return _m.iter; }
	Iterator         base() &&       { return std::move(_m.iter); }

	reference      operator*()
	{
		return static_cast<UnaryFunc &>(_m.second())(*_m.iter);
	}
	decltype(auto) operator*() const
	{
		return static_cast<const UnaryFunc &>(_m.second())(*_m.iter);
	}

	transform_iterator & operator++()  OEL_ALWAYS_INLINE
	{	// pre-increment
		++_m.iter;
		return *this;
	}

	template< typename C = iterator_category,
	          enable_if< _detail::isForward<C> > = 0
	>
	transform_iterator operator++(int) &
	{	// post-increment
		auto tmp = *this;
		++_m.iter;
		return tmp;
	}

	// For bidirectional Iterator
	transform_iterator & operator--()  OEL_ALWAYS_INLINE
	{
		--_m.iter;
		return *this;
	}

	transform_iterator operator--(int) &
	{
		auto tmp = *this;
		--_m.iter;
		return tmp;
	}

	// For random-access Iterator
	transform_iterator & operator+=(difference_type offset) &
	{
		_m.iter += offset;
		return *this;
	}

	transform_iterator & operator-=(difference_type offset) &
	{
		_m.iter -= offset;
		return *this;
	}

	friend transform_iterator operator +(difference_type offset, transform_iterator it)  { return it += offset; }
	friend transform_iterator operator +(transform_iterator it, difference_type offset)  { return it += offset; }
	friend transform_iterator operator -(transform_iterator it, difference_type offset)  { return it -= offset; }

	difference_type operator -(const transform_iterator & right) const               { return _m.iter - right._m.iter; }
	template< typename Sentinel,
		decltype(std::declval<Iterator>() == std::declval<Sentinel>()) = true // enable if sentinel for Iterator
	>
	friend difference_type operator -(const transform_iterator & left, Sentinel right)  { return left._m.iter - right; }
	template< typename Sentinel >
	friend difference_type operator -(Sentinel left, const transform_iterator & right)  { return left - right._m.iter; }

	reference operator[](difference_type offset) const
	{
		auto tmp = *this;
		tmp += offset; // not operator + to save a call in non-optimized builds
		return *tmp;
	}

	bool        operator==(const transform_iterator & right) const        { return _m.iter == right._m.iter; }
	bool        operator!=(const transform_iterator & right) const        { return _m.iter != right._m.iter; }

	bool        operator <(const transform_iterator & right) const        { return _m.iter < right._m.iter; }
	bool        operator >(const transform_iterator & right) const        { return _m.iter > right._m.iter; }
	bool        operator<=(const transform_iterator & right) const        { return !(_m.iter > right._m.iter); }
	bool        operator>=(const transform_iterator & right) const        { return !(_m.iter < right._m.iter); }

	template< typename Sentinel >
	friend bool operator==(const transform_iterator & left, Sentinel right)  { return left._m.iter == right; }
	template< typename Sentinel >
	friend bool operator==(Sentinel left, const transform_iterator & right)  { return right._m.iter == left; }
	template< typename Sentinel >
	friend bool operator!=(const transform_iterator & left, Sentinel right)  { return left._m.iter != right; }
	template< typename Sentinel >
	friend bool operator!=(Sentinel left, const transform_iterator & right)  { return right._m.iter != left; }

	template< typename Sentinel >
	friend bool operator <(const transform_iterator & left, Sentinel right)  { return left._m.iter < right; }
	template< typename Sentinel >
	friend bool operator <(Sentinel left, const transform_iterator & right)  { return right._m.iter > left; }
	template< typename Sentinel >
	friend bool operator >(const transform_iterator & left, Sentinel right)  { return left._m.iter > right; }
	template< typename Sentinel >
	friend bool operator >(Sentinel left, const transform_iterator & right)  { return right._m.iter < left; }
	template< typename Sentinel >
	friend bool operator<=(const transform_iterator & left, Sentinel right)  { return !(left._m.iter > right); }
	template< typename Sentinel >
	friend bool operator<=(Sentinel left, const transform_iterator & right)  { return !(right._m.iter < left); }
	template< typename Sentinel >
	friend bool operator>=(const transform_iterator & left, Sentinel right)  { return !(left._m.iter < right); }
	template< typename Sentinel >
	friend bool operator>=(Sentinel left, const transform_iterator & right)  { return !(right._m.iter > left); }
};

} // namespace oel
