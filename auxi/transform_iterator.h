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
	template< typename I, typename F,
	          bool = std::is_empty<F>::value >
	struct TightPair
	{
		I iter;
		F _fun;

		OEL_ALWAYS_INLINE const F & func() const noexcept { return _fun; }
		OEL_ALWAYS_INLINE       F & func() noexcept       { return _fun; }
	};

	template< typename Iterator_MSVC_needs_unique_name, typename Empty_function_object_MSVC_name >
	struct TightPair< Iterator_MSVC_needs_unique_name, Empty_function_object_MSVC_name, true >
	 :	Empty_function_object_MSVC_name
	{
		Iterator_MSVC_needs_unique_name iter;

		TightPair(Iterator_MSVC_needs_unique_name i, Empty_function_object_MSVC_name f)
		 :	Empty_function_object_MSVC_name(f), iter{std::move(i)} {
		}

		OEL_ALWAYS_INLINE const Empty_function_object_MSVC_name & func() const noexcept { return *this; }
		OEL_ALWAYS_INLINE       Empty_function_object_MSVC_name & func() noexcept       { return *this; }
	};


	template< typename I, typename F,
	          bool = std::is_move_assignable<F>::value >
	struct TransformIterHelp
	{
		TightPair<I, F> pair;
	};

	template< typename I, typename F >
	struct TransformIterHelp<I, F, false>
	{
		TightPair<I, F> pair;

		TransformIterHelp(TightPair<I, F> && p)  : pair{std::move(p)} {}

		TransformIterHelp(TransformIterHelp &&) = default;
		TransformIterHelp(const TransformIterHelp &) = default;

		static_assert( std::is_nothrow_copy_constructible<F>::value and std::is_trivially_destructible<F>::value,
			"Move assignable, or noexcept copy constructible and trivially destructible UnaryFunc required" );

		TransformIterHelp & operator =(TransformIterHelp && other) &
			noexcept(std::is_nothrow_move_assignable<I>::value)
		{
			pair.iter = std::move(other.pair.iter);
			// Lambdas generally not assignable, so using constructor
			void * fun = &pair.func();
			new(fun) F{other.pair.func()};

			return *this;
		}

		TransformIterHelp & operator =(const TransformIterHelp & other) &
		{
			pair.iter = other.pair.iter;

			void * fun = &pair.func();
			new(fun) F{other.pair.func()};

			return *this;
		}
	};
}


/** @brief Similar to boost::transform_iterator
*
* Lambdas supported as long as they are not mutable and by-value captures are
* nothrow copy constructible and trivially destructible. */
template< typename UnaryFunc, typename Iterator >
class transform_iterator  : private _detail::TransformIterHelp<Iterator, UnaryFunc>
{
	using _help = _detail::TransformIterHelp<Iterator, UnaryFunc>;
	using _help::pair;

public:
	using iterator_category = std::conditional_t<
			iter_is_forward<Iterator>::value and
				std::is_copy_constructible<UnaryFunc>::value,
			std::forward_iterator_tag,
			std::input_iterator_tag >;

	using difference_type = iter_difference_t<Iterator>;
	using reference       = decltype( pair.func()(*pair.iter) );
	using pointer         = void;
	using value_type      = std::remove_cv_t< std::remove_reference_t<reference> >;

	transform_iterator(UnaryFunc f, Iterator it)  : _help{{ std::move(it), std::move(f) }} {}

	const Iterator & base() const &  { return pair.iter; }
	Iterator         base() &&       { return std::move(pair.iter); }

	reference operator*() const
	{
		return pair.func()(*pair.iter);
	}

	transform_iterator & operator++()  OEL_ALWAYS_INLINE
	{	// pre-increment
		++pair.iter;
		return *this;
	}

	template< typename C = iterator_category,
	          enable_if< ! std::is_same<C, std::input_iterator_tag>::value > = 0
	>
	transform_iterator operator++(int) &
	{
		auto tmp = *this;
		++pair.iter;
		return tmp;
	}

	template< typename C = iterator_category,
	          enable_if< std::is_same<C, std::input_iterator_tag>::value > = 0
	>
	void operator++(int) &
	{
		++pair.iter;
	}

	template< typename Sentinel >  OEL_ALWAYS_INLINE
	friend bool operator==(const transform_iterator & left, Sentinel right)  { return left.pair.iter == right; }
	template< typename Sentinel >  OEL_ALWAYS_INLINE
	friend bool operator!=(const transform_iterator & left, Sentinel right)  { return left.pair.iter != right; }
	template< typename Sentinel >  OEL_ALWAYS_INLINE
	friend bool operator==(Sentinel left, const transform_iterator & right)  { return left == right.pair.iter; }
	template< typename Sentinel >  OEL_ALWAYS_INLINE
	friend bool operator!=(Sentinel left, const transform_iterator & right)  { return left != right.pair.iter; }
};

} // namespace oel
