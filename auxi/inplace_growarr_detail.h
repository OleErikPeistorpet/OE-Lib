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
		Size _size{};
		storage_for<T> _data[Capacity];


		T *       data() noexcept       { return reinterpret_cast<T *>(_data); }
		const T * data() const noexcept { return reinterpret_cast<const T *>(_data); }


		template< bool ForceMemcpy = false, typename InputIter >
		InputIter doAssign(InputIter src, Size const count)
		{
			if constexpr (ForceMemcpy or can_memmove_with<T *, InputIter>)
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

		InplaceGrowarrSpecial(const InplaceGrowarrSpecial & other)
		{
			this->_size = other._size;
			_detail::UninitCopy(other.data(), other._size, this->data());
		}

		InplaceGrowarrSpecial(InplaceGrowarrSpecial && other) noexcept
		{
			this->_size = other._size;
			_detail::Relocate(other.data(), other._size, this->data());
			other._setEmptyIf< is_trivially_relocatable<T>::value >();
		}

		InplaceGrowarrSpecial & operator =(InplaceGrowarrSpecial && other) &
			 noexcept(std::is_nothrow_move_assignable_v<T> or is_trivially_relocatable<T>::value)
		{
			constexpr auto trivialReloc = is_trivially_relocatable<T>::value;
		#if OEL_HAS_EXCEPTIONS
			static_assert( std::is_nothrow_move_constructible_v<T> or trivialReloc,
				"Containers in oel require that T is noexcept move constructible or trivially relocatable" );
		#endif
			this->doAssign<trivialReloc>(std::make_move_iterator(other.data()), other._size);
			other._setEmptyIf<trivialReloc>();
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

		template< bool B >
		void _setEmptyIf()
		{
			if constexpr (B)
				this->_size = 0;
		}
	};
}

} // namespace oel
