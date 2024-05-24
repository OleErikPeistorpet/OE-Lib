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
	InputIter UninitCopyA(InputIter src, size_t n, T * dest, false_type)
	{
		allocator<T> a{};
		return _detail::UninitCopy(src, dest, dest + n, a);
	}

	template< typename ContiguousIter, typename T >
	void UninitCopyA(ContiguousIter src, size_t n, T * dest, true_type)
	{
		_detail::MemcpyCheck(src, n, dest);
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
		Size _size{};
		storage_for<T> _data[Capacity];


		T *       data() noexcept       { return reinterpret_cast<T *>(_data); }
		const T * data() const noexcept { return reinterpret_cast<const T *>(_data); }


		template< typename ContiguousIter >
		ContiguousIter doAssign(ContiguousIter first, Size const count, true_type)
		{	// fastest assign
			_size = count;
			// Not portable for self assignment. Use memmove?
			_detail::MemcpyCheck(first, count, _data);

			return first + count;
		}

		template< typename InputIter >
		InputIter doAssign(InputIter src, Size const count, false_type)
		{	// cannot use memcpy
			auto cpy = [](InputIter src_, T * dest, T * dLast)
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
			_detail::UninitCopyA(other.data(), other._size, this->data(), std::is_trivially_copy_constructible<T>());
		}

		InplaceGrowarrSpecial(InplaceGrowarrSpecial && other) noexcept
		{
			static_assert( std::is_nothrow_move_constructible_v<T> or is_trivially_relocatable<T>::value,
				"inplace_growarr(inplace_growarr &&) requires noexcept move constructible or trivially relocatable T" );
			this->_size = other._size;
			_detail::UninitCopyA(
				std::make_move_iterator(other.data()), other._size, this->data(),
				bool_constant< is_trivially_relocatable<T>::value or std::is_trivially_move_constructible_v<T> >{} );
			other.setEmptyIf(is_trivially_relocatable<T>());
		}

		InplaceGrowarrSpecial & operator =(InplaceGrowarrSpecial && other) &
			 noexcept(std::is_nothrow_move_constructible_v<T> or is_trivially_relocatable<T>::value)
		{
			this->doAssign(std::make_move_iterator(other.data()), other._size, is_trivially_relocatable<T>());
			other.setEmptyIf(is_trivially_relocatable<T>());
			return *this;
		}

		InplaceGrowarrSpecial & operator =(const InplaceGrowarrSpecial & other) &
		{
			this->doAssign(other.data(), other._size, false_type{});
			return *this;
		}

		~InplaceGrowarrSpecial() noexcept
		{
			_detail::Destroy(this->data(), this->data() + this->_size);
		}

		void setEmptyIf(true_type) { this->_size = 0; }

		OEL_ALWAYS_INLINE void setEmptyIf(false_type) {}
	};
}

} // namespace oel
