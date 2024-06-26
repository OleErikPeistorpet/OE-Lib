#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/impl_algo.h"
#include "optimize_ext/default.h"
#include "allocator.h"


namespace oel
{
namespace _detail
{
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


	template< typename T, typename Size >
	struct InplaceGrowarrProxy
	{
		Size size;
		T    data[1];

		bool derefValid(const T * pos) const
		{
			return static_cast<size_t>(pos - data) < static_cast<size_t>(size);
		}
	};

	template< typename T, size_t Capacity, typename Size >
	struct InplaceGrowarrBase
	{
		Size           _size{};
		storage_for<T> _data[Capacity];


		T *       data() noexcept       { return reinterpret_cast<T *>(_data); }
		const T * data() const noexcept { return reinterpret_cast<const T *>(_data); }


		template< typename InputIter >
		InputIter doAssign(InputIter src, Size const count)
		{
			if constexpr (can_memmove_with<T *, InputIter>)
			{
				_size = count;
				_detail::MemcpyCheck(src, count, _data);

				return src + count;
			}
			else
			{	auto cpy = [](InputIter src_, T *__restrict dest, T * dLast)
				{
					while (dest != dLast)
					{
						*dest = *src_;
						++src_; ++dest;
					}
					return src_;
				};
				if (_size < count)
				{	// assign to old elements as far as we can
					src = cpy(src, data(), data() + _size);
					while (_size < count)
					{	// each iteration updates _size for exception safety
						::new(static_cast<void *>(data() + _size)) T(*src);
						++src; ++_size;
					}
				}
				else // downsizing, assign new and destroy rest
				{	T *const newEnd = data() + count;
					src = cpy(src, data(), newEnd);
					_detail::Destroy(newEnd, data() + _size);
					_size = count;
				}
				return src;
			}
		}
	};


	template< typename T, size_t C, typename S,
	          bool = std::is_trivially_copyable_v<T>
	>
	struct InplaceGrowarrSpecial : InplaceGrowarrBase<T, C, S> {};

	template< typename T, size_t Capacity, typename Size >
	struct InplaceGrowarrSpecial<T, Capacity, Size, false>
	 :	InplaceGrowarrBase<T, Capacity, Size>
	{
		constexpr InplaceGrowarrSpecial() = default;

		InplaceGrowarrSpecial(InplaceGrowarrSpecial && other) noexcept
		{
			_relocateFrom(other);
		}

		InplaceGrowarrSpecial(const InplaceGrowarrSpecial & other)
		{
			this->_size = other._size;
			_detail::UninitCopy(other.data(), other._size, this->data());
		}

		InplaceGrowarrSpecial & operator =(InplaceGrowarrSpecial && other) &
			 noexcept(std::is_nothrow_move_assignable_v<T> or is_trivially_relocatable<T>::value)
		{
			if constexpr (is_trivially_relocatable<T>::value)
			{
				_detail::Destroy(this->data(), this->data() + this->_size);
				_relocateFrom(other);
			}
			else
			{	(void) AssertNothrowMoveConstruct<T>{};
				this->doAssign(std::move_iterator{other.data()}, other._size);
			}
			return *this;
		}

		InplaceGrowarrSpecial & operator =(const InplaceGrowarrSpecial & other) &
		{
			if (this != &other)
				this->doAssign(other.data(), other._size);

			return *this;
		}

		~InplaceGrowarrSpecial() noexcept
		{
			_detail::Destroy(this->data(), this->data() + this->_size);
		}

		void _relocateFrom(InplaceGrowarrSpecial & other) noexcept
		{
			_detail::Relocate(other.data(), other._size, this->data());
			this->_size = other._size;
			if constexpr (is_trivially_relocatable<T>::value)
				other->_size = 0;
		}
	};
}

} // namespace oel
