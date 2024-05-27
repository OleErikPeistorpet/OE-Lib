#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h" // for TightPair


namespace oel::_detail
{
	template< typename T,
	          bool = std::is_move_assignable_v<T> >
	class AssignableWrap
	{
		static_assert( std::is_trivially_copy_constructible_v<T> and std::is_trivially_destructible_v<T>,
			"The user-supplied function must be move assignable, or trivially copy constructible and trivially destructible" );

		union Impl
		{
			struct {} _none;
			T _val;

			constexpr Impl() noexcept : _none{} {}
			constexpr Impl(const T & src) noexcept : _val(src) {}

			Impl(const Impl &) = default;

			void operator =(const Impl & other) & noexcept
			{
				::new(this) Impl(other);
			}

			OEL_ALWAYS_INLINE constexpr operator const T &() const { return _val; }
			OEL_ALWAYS_INLINE constexpr operator       T &()       { return _val; }
		};

		using EmptyType_7KQw = T;

		struct ImplEmpty : EmptyType_7KQw
		{
			constexpr ImplEmpty(EmptyType_7KQw src) noexcept
			 :	EmptyType_7KQw(src) {}

			ImplEmpty() = default;
			ImplEmpty(const ImplEmpty &) = default;

			OEL_ALWAYS_INLINE constexpr void operator =(const ImplEmpty &) & noexcept {}
		};

	public:
		using Type = std::conditional_t< std::is_empty_v<T>, ImplEmpty, Impl >;
	};

	template< typename T >
	class AssignableWrap<T, true>
	{
	public:
		using Type = T;
	};


	template< typename T, typename Func, bool /*IsConstCallable*/ >
	struct IterWithFuncBase
	{
		using FnRef = const Func &;

		static constexpr auto canCallConst = true;

		TightPair< T, typename AssignableWrap<Func>::Type > m;
	};

	template< typename T, typename Func >
	struct IterWithFuncBase<T, Func, false>
	{
		using FnRef = Func &;

		static constexpr auto canCallConst = false;

		TightPair< T, typename AssignableWrap<Func>::Type > mutable m;
	};
}