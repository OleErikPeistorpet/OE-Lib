#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"

#include <cstring>
#include <stdexcept>


// std::max not constexpr for GCC 4
template<typename T>
constexpr T oel_max(const T & a, const T & b)
{
	return a < b ? b : a;
}


namespace oel
{
namespace _detail
{
	struct Throw
	{	// Exception throwing has been split out from templates to avoid bloat
		[[noreturn]] static void OutOfRange(const char * what)
		{
			OEL_THROW(std::out_of_range(what), what);
		}

		[[noreturn]] static void LengthError(const char * what)
		{
			OEL_THROW(std::length_error(what), what);
		}
	};



	template<typename T> struct AssertTrivialRelocate
	{
		static_assert(is_trivially_relocatable<T>::value,
			"The function requires trivially relocatable T, see declaration of is_trivially_relocatable");
	};



	template<typename ContiguousIter>
	inline void MemcpyCheck(ContiguousIter const src, size_t const nElems, void *const dest)
	{	// memcpy(nullptr, nullptr, 0) is UB. The trouble is that checking can have significant performance hit.
		// GCC known to need the check in some cases
	#if !defined _MSC_VER or _MSC_VER >= 2000 or OEL_MEM_BOUND_DEBUG_LVL
		if (nElems > 0)
	#endif
		{	// Dereference to detect out of range errors if the iterator has internal check
		#if OEL_MEM_BOUND_DEBUG_LVL
			(void) *src;
			(void) *(src + (nElems - 1));
		#endif
			std::memcpy(dest, to_pointer_contiguous(src), sizeof(*src) * nElems);
		}
	}


	template<typename T, typename Alloc, bool B, typename... Args>
	inline auto ConstructImpl(bool_constant<B>, Alloc & a, T *__restrict p, Args &&... args)
	->	decltype( a.construct(p, static_cast<Args &&>(args)...) )
		        { a.construct(p, static_cast<Args &&>(args)...); }
	// void * worse match than T *
	template<typename T, typename Alloc, typename... Args>
	inline void ConstructImpl(std::true_type, Alloc &, void *__restrict p, Args &&... args)
	{	// T constructible from Args
		::new(p) T(static_cast<Args &&>(args)...);
	}
	// list-initialization
	template<typename T, typename Alloc, typename... Args>
	inline void ConstructImpl(std::false_type, Alloc &, void *__restrict p, Args &&... args)
	{
		::new(p) T{static_cast<Args &&>(args)...};
	}

	template<typename Alloc, typename T, typename... Args>
	OEL_ALWAYS_INLINE inline void Construct(Alloc & a, T *__restrict p, Args &&... args)
	{
		ConstructImpl<T>(std::is_constructible<T, Args...>(),
		                 a, p, static_cast<Args &&>(args)...);
	}

	template<typename T, typename... Args>
	OEL_ALWAYS_INLINE inline void ConstructA(T *__restrict p, Args &&... args)
	{
		struct {} a;
		Construct(a, p, static_cast<Args &&>(args)...);
	}


	template<typename T>
	void Destroy(T * first, const T * last) noexcept
	{	// first > last is OK, does nothing
		OEL_CONST_COND if (!std::is_trivially_destructible<T>::value) // for speed with non-optimized builds
		{
			for (; first < last; ++first)
				first-> ~T();
		}
	}


	template<typename Alloc, typename ContiguousIter, typename T,
	         enable_if< can_memmove_with<T *, ContiguousIter>::value > = 0>
	inline void UninitCopy(ContiguousIter src, T * dFirst, T * dLast, Alloc &)
	{
		_detail::MemcpyCheck(src, dLast - dFirst, dFirst);
	}

	template<typename Alloc, typename InputIter, typename T,
	         enable_if< not can_memmove_with<T *, InputIter>::value > = 0>
	InputIter UninitCopy(InputIter src, T * dest, T *const dLast, Alloc & alloc)
	{
		T *const dFirst = dest;
		OEL_TRY_
		{
			while (dest != dLast)
			{
				_detail::Construct(alloc, dest, *src);
				++src; ++dest;
			}
		}
		OEL_CATCH_ALL
		{
			_detail::Destroy(dFirst, dest);
			OEL_WHEN_EXCEPTIONS_ON(throw);
		}
		return src;
	}


	template<typename Alloc>
	struct UninitFill
	{
		template<typename T>
		using IsByte = bool_constant< sizeof(T) == 1 and
			(std::is_integral<T>::value or std::is_enum<T>::value) >;

		template<typename T, typename... Args,
		         enable_if< !IsByte<T>::value > = 0>
		void operator()(T * first, T *const last, Alloc & alloc, const Args &... args) const
		{
			T *const init = first;
			OEL_TRY_
			{
				for (; first != last; ++first)
					_detail::Construct(alloc, first, args...);
			}
			OEL_CATCH_ALL
			{
				_detail::Destroy(init, first);
				OEL_WHEN_EXCEPTIONS_ON(throw);
			}
		}

		template<typename T, enable_if< IsByte<T>::value > = 0>
		void operator()(T * first, T * last, Alloc &, T val) const
		{
			std::memset(first, static_cast<int>(val), last - first);
		}

		template<typename T, enable_if< std::is_trivial<T>::value > = 0>
		void operator()(T * first, T * last, Alloc &) const
		{
			std::memset(first, 0, sizeof(T) * (last - first));
		}
	};

	template<typename Alloc, typename T>
	inline void UninitDefaultConstruct(T *const first, T *const last, Alloc & a)
	{
		OEL_CONST_COND if (!is_trivially_default_constructible<T>::value)
			UninitFill<Alloc>{}(first, last, a);
	}

	struct UninitFillA
	{
		template<typename T, typename... Arg>
		void operator()(T *const first, T *const last, const Arg &... arg) const
		{
			struct A {} a;
			UninitFill<A>{}(first, last, a, arg...);
		}
	};

	template<typename T>
	inline void UninitDefaultConstructA(T *const first, T *const last)
	{
		struct {} a;
		UninitDefaultConstruct(first, last, a);
	}



	template<typename Range, typename IterTrav> // pass dummy int to prefer this overload
	auto SizeOrEndImpl(const Range & r, IterTrav, int)
	->	decltype((size_t) oel::ssize(r)) { return oel::ssize(r); }

	template<typename Range>
	size_t SizeOrEndImpl(const Range & r, forward_traversal_tag, long)
	{
		size_t n = 0;
		auto it = begin(r);  auto const last = end(r);
		while (it != last)
		{ ++n; ++it; }
		return n;
	}

	template<typename Range>
	auto SizeOrEndImpl(const Range & r, single_pass_traversal_tag, long) { return end(r); }

	// If r is sized or multi-pass, returns element count as size_t, else end(r)
	template<typename Range>
	auto SizeOrEnd(const Range & r)
	{
		using It = decltype(begin(r));
		return _detail::SizeOrEndImpl(r, iter_traversal_t<It>(), 0);
	}
}

} // namespace oel
