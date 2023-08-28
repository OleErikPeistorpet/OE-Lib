#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/algo_detail.h"
#include "optimize_ext/default.h"
#include "align_allocator.h"


namespace oel
{
namespace _detail
{
	template< typename InputIter, typename T >
	void UninitCopyA(InputIter src, size_t n, T * dest, false_type)
	{
		struct {} a;
		_detail::UninitCopy(src, dest, dest + n, a);
	}

	template< typename ContiguousIter, typename T >
	void UninitCopyA(ContiguousIter src, size_t n, T * dest, true_type)
	{
		_detail::MemcpyCheck(src, n, dest);
	}


	template< typename T, typename Size >
	struct InplaceDynarrProxy
	{
		Size size;
		T    data[1];

		bool derefValid(const T * pos) const
		{
			return static_cast<size_t>(pos - data) < static_cast<size_t>(size);
		}
	};

	template< typename T, size_t Capacity, typename Size >
	struct InplaceDynarrBase
	{
		Size _size{};
		aligned_union_t<T> _data[Capacity];


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
			auto copy = [](InputIter src_, T * dest, T * dLast)
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
				src = copy(src, data(), data() + _size);
				while (_size < count)
				{	// each iteration updates _size for exception safety
					_detail::ConstructA(data() + _size, *src);
					++src; ++_size;
				}
			}
			else // downsizing, assign new and destroy rest
			{	T *const newEnd = data() + count;
				src = copy(src, data(), newEnd);
				_detail::Destroy(newEnd, data() + _size);
				_size = count;
			}
			return src;
		}
	};


	template< typename T, size_t C, typename S,
		bool = is_trivially_copyable<T>::value >
	struct InplaceDynarrSpecial : InplaceDynarrBase<T, C, S> {};

	template< typename T, size_t Capacity, typename Size >
	struct InplaceDynarrSpecial<T, Capacity, Size, false>
	 :	InplaceDynarrBase<T, Capacity, Size>
	{
		constexpr InplaceDynarrSpecial() = default;

		InplaceDynarrSpecial(const InplaceDynarrSpecial & other)
		{
			this->_size = other._size;
			struct {} a;
			_detail::UninitCopy(other.data(), data(), data() + _size, a);
		}

		InplaceDynarrSpecial(InplaceDynarrSpecial && other)
			noexcept(std::is_nothrow_move_constructible<T>::value or is_trivially_relocatable<T>::value)
		{
			this->_size = other._size;
			_detail::UninitCopyA(
				std::make_move_iterator(other.data()), other._size, data(),
				is_trivially_relocatable<T>() );
			other.setEmptyIf(is_trivially_relocatable<T>());
		}

		InplaceDynarrSpecial & operator =(InplaceDynarrSpecial && other) &
		{
			this->doAssign(std::make_move_iterator(other.data()), other._size, is_trivially_relocatable<T>());
			other.setEmptyIf(is_trivially_relocatable<T>());
			return *this;
		}

		InplaceDynarrSpecial & operator =(const InplaceDynarrSpecial & other) &
		{
			this->doAssign(other.data(), other._size, is_trivially_copyable<T>());
			return *this;
		}

		~InplaceDynarrSpecial() noexcept
		{
			_detail::Destroy(data(), data() + this->_size);
		}

		void setEmptyIf(true_type) { this->_size = 0; }

		OEL_ALWAYS_INLINE void setEmptyIf(false_type) {}
	};
}

} // namespace oel
