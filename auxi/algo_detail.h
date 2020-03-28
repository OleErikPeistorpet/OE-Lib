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
	{	// at namespace scope this produces warnings of unreferenced function or failed inlining
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
		// GCC 4.9 and up known to need the check in some cases
	#if (!defined __GNUC__ and !defined _MSC_VER) or OEL_GCC_VERSION >= 409 or _MSC_VER >= 2000 or OEL_MEM_BOUND_DEBUG_LVL
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


	template<typename Alloc, typename T>
	struct Construct
	{
		template<typename... Args, typename Alloc_ = Alloc>
		auto operator()(Alloc_ & a, T *__restrict p, Args &&... args) const
		 -> decltype( a.construct(p, static_cast<Args &&>(args)...) )
		            { a.construct(p, static_cast<Args &&>(args)...); }
		// void * worse match than T *
		template<typename... Args,
		         enable_if< std::is_constructible<T, Args...>::value > = 0>
		void operator()(Alloc &, void *__restrict p, Args &&... args) const
		{	// T constructible from Args
			::new(p) T(static_cast<Args &&>(args)...);
		}

		template<typename... Args,
		         enable_if< !std::is_constructible<T, Args...>::value > = 0>
		void operator()(Alloc &, void *__restrict p, Args &&... args) const
		{	// list-initialize
			::new(p) T{static_cast<Args &&>(args)...};
		}
	};


	template<typename Ptr, typename ConstPtr>
	void Destroy(Ptr first, ConstPtr const last) noexcept
	{
		using T = typename std::pointer_traits<Ptr>::element_type;
		OEL_CONST_COND if (!std::is_trivially_destructible<T>::value) // for speed with non-optimized builds
		{
			T *       p = _detail::ToAddress(first);
			const T * e = _detail::ToAddress(last);
			for (; p < e; ++p)
				p-> ~T();
		}
	}


	template<typename Alloc, typename ContiguousIter, typename T,
	         enable_if< can_memmove_with<T *, ContiguousIter>::value > = 0>
	inline void UninitCopy(ContiguousIter src, size_t count, T * dest, Alloc &)
	{
		_detail::MemcpyCheck(src, count, dest);
	}

	template<typename Alloc, typename InputIter, typename T,
	         enable_if< !can_memmove_with<T *, InputIter>::value > = 0>
	InputIter UninitCopy(InputIter src, size_t const count, T * dest, Alloc & alloc)
	{
		T *const dFirst = dest;
		OEL_TRY_
		{
			const T * dLast = dest + count;
			while (dest != dLast)
			{
				Construct<Alloc, T>{}(alloc, dest, *src);
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
		void operator()(T * first, size_t const count, Alloc & alloc, const Args &... args) const
		{
			T *const init = first;
			OEL_TRY_
			{
				for (const T * last = first + count; first != last; ++first)
					Construct<Alloc, T>{}(alloc, first, args...);
			}
			OEL_CATCH_ALL
			{
				_detail::Destroy(init, first);
				OEL_WHEN_EXCEPTIONS_ON(throw);
			}
		}

		template<typename T, enable_if< IsByte<T>::value > = 0>
		void operator()(T * first, size_t count, Alloc &, T val) const
		{
			std::memset(first, static_cast<int>(val), count);
		}

		template<typename T, enable_if< std::is_trivial<T>::value > = 0>
		void operator()(T * first, size_t count, Alloc &) const
		{
			std::memset(first, 0, sizeof(T) * count);
		}
	};

	template<typename Alloc, typename T>
	inline void UninitDefaultConstruct(T *const first, size_t const count, Alloc & a)
	{
		OEL_CONST_COND if (!is_trivially_default_constructible<T>::value)
			UninitFill<Alloc>{}(first, count, a);
	}
}

} // namespace oel
