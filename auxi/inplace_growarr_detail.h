#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/impl_algo.h"
#include "allocator.h"


namespace oel::_detail
{
	template< typename Iterator >
	struct TryReturn
	{
		bool     success;
		Iterator srcLast;
	};


	template< typename InputIter, typename T >
	InputIter UninitCopy(InputIter src, size_t const n, T *__restrict dest)
	{
		if constexpr (can_memmove_with<T *, InputIter>)
		{
			_detail::MemcpyCheck(src, n, dest);
			return src + n;
		}
		else
		{	T *const dFirst = dest;
			OEL_TRY_
			{
				for (size_t i{}; i < n; ++i)
				{
					::new(static_cast<void *>(dest + i)) T(*src);
					++src;;
				}
			}
			OEL_CATCH_ALL
			{
				_detail::Destroy(dFirst, dest);
				OEL_RETHROW;
			}
			return src;
		}
	}

	struct UninitFillA
	{
		template< typename T, typename... Args >
		static void call(T *__restrict first, T *const last, const Args &... args)
		{
			using A = allocator<>;
			UninitFill<A>::call(first, last, A{}, args...);
		}
	};

	struct UninitDefaultConstructA
	{
		template< typename T >
		static void call(T *__restrict first, T *const last)
		{
			using A = allocator<>;
			A a{};
			DefaultInit<A>::call(first, last, a);
		}
	};


	template< typename Integer, typename T, typename ContiguousIter >
	Integer Erase(Integer const size, T *const data, ContiguousIter pos)
	{
		T *const ptr{oel::to_pointer_contiguous(pos)};
		OEL_ASSERT(data <= ptr and ptr < data + size);

		if constexpr (is_trivially_relocatable<T>::value)
		{
			ptr-> ~T();
			auto const next = ptr + 1;
			auto const nAfter = size - (next - data);
			std::memmove( // relocate [pos + 1, end) to [pos, end - 1)
				static_cast<void *>(ptr),
				static_cast<const void *>(next),
				sizeof(T) * nAfter );
		}
		else
		{	auto last = std::move(ptr + 1, data + size, ptr);
			last-> ~T();
		}
		return size - 1;
	}

	template< typename Integer, typename T, typename ContiguousIter, typename ContiguousIter2 >
	Integer Erase(Integer const size, T *const data, ContiguousIter first, ContiguousIter2 last)
	{
		T *            dest{oel::to_pointer_contiguous(first)};
		const T *const pLast{oel::to_pointer_contiguous(last)};
		OEL_ASSERT(data <= dest and dest <= pLast and pLast <= data + size);

		auto const nErase = pLast - dest;
		if constexpr (is_trivially_relocatable<T>::value)
		{
			_detail::Destroy(dest, pLast);
			auto const nAfter = size - (pLast - data);
			std::memmove( // relocate [last, end) to [first, first + nAfter)
				static_cast<void *>(dest),
				static_cast<const void *>(pLast),
				sizeof(T) * nAfter );
		}
		else if (0 < nErase) // must avoid self-move-assigning the elements
		{
			auto const end = data + size;
			dest = std::move(const_cast<T *>(pLast), end, dest);
			_detail::Destroy(dest, end);
		}
		return size - nErase;
	}
}