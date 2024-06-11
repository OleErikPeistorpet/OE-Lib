#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/array_interface.h"
#include "auxi/array_iterator.h"
#include "auxi/inplace_growarr_detail.h"

/** @file
*/

namespace oel
{

template< size_t Capacity, typename Size >
struct _toInplaceGrowarrFn
{
	template< typename InputRange >
	friend auto operator |(InputRange && r, _toInplaceGrowarrFn)
		{
			using T = iter_value_t< iterator_t<InputRange> >;
			return inplace_growarr<T, Capacity, Size>(from_range, static_cast<InputRange &&>(r));
		}

	template< typename InputRange >
	auto operator()(InputRange && r) const   { return static_cast<InputRange &&>(r) | *this; }
};
//! `to_inplace_growarr<C>` is same as `std::ranges::to< inplace_growarr<T, C> >()` with T deduced from `r`
template< size_t Capacity, typename Size = size_t >
inline constexpr _toInplaceGrowarrFn<Capacity, Size> to_inplace_growarr;

//! Can be used to deduce T from val: `make_inplace_growarr<Capacity>(size, val)`
template< size_t Capacity, typename T >
inplace_growarr<T, Capacity> make_inplace_growarr(size_t size, const T & val)
	{
		inplace_growarr<T, Capacity> res{};
		res.append(size, val);
		return res;
	}

//! inplace_growarr is trivially relocatable if T is
template< typename T, size_t C, typename S >
is_trivially_relocatable<T> specify_trivial_relocate(inplace_growarr<T, C, S>);

/**
* @brief Resizable array, statically allocated. Specify maximum size as template argument.
*
* Behaviour which equals that of std::vector is mostly not documented. */
template< typename T, size_t Capacity, typename Size >
class inplace_growarr
 :	public _arrayInterface< inplace_growarr<T, Capacity, Size> >, private _detail::InplaceGrowarrSpecial<T, Capacity, Size>
{
	static_assert(Capacity <= std::numeric_limits<Size>::max(), "Capacity does not fit in type Size");

	using _base = typename inplace_growarr::InplaceGrowarrBase;

public:
	using value_type      = T;
	using size_type       = Size;
	using difference_type = ptrdiff_t;

#if OEL_MEM_BOUND_DEBUG_LVL
	using iterator       = debug::array_iterator< T *, _detail::InplaceGrowarrProxy<T, Size> >;
	using const_iterator = debug::array_iterator< const T *, _detail::InplaceGrowarrProxy<T, Size> >;
#else
	using iterator       = T *;
	using const_iterator = const T *;
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	inplace_growarr() = default;

	/** @brief Elements are default initialized, meaning non-class T produces indeterminate values
	* @throw bad_alloc if size > Capacity  */
	inplace_growarr(size_type size, for_overwrite_t)
		{
			if (Capacity < size)
				_detail::BadAlloc::raise();

			_detail::UninitDefaultConstructA::call(data(), data() + size);
			_size = size;
		}
	//! Throws bad_alloc if size > Capacity. (Elements are value-initialized, same as std::vector)
	explicit inplace_growarr(size_type size)
		{
			if (Capacity < size)
				_detail::BadAlloc::raise();

			_detail::UninitFillA::call(data(), data() + size);
			_size = size;
		}

	//! T can be deduced - TODO more description
	template< typename InputRange >
	inplace_growarr(from_range_t, InputRange && source)   { append(source); }

	inplace_growarr(std::initializer_list<T> il)          { append(il); }

	inplace_growarr & operator =(std::initializer_list<T> il) &  { assign(il);  return *this; }

	// TODO: try_assign, try_append, try_insert_range, and unchecked_push_back, unchecked_emplace_back

	/**
	* @brief Replace the contents with source
	* @throw bad_alloc if count > Capacity  */
	template< typename InputRange >
	auto assign(InputRange && source)
	->  iterator_t<InputRange>                    { return _assign(_getSize(source, 0), source); }

	void assign(size_type count, const T & val)   { clear();  append(count, val); }

	/**
	* @brief Add at end the elements from range (in order)
	* @param source object which begin and end can be called on (an array, STL container or iterator_range)
	* @return begin(source) incremented to end of source
	*
	* Any previous end iterator will point to the first element added.
	* Strong exception safety, aka. commit or rollback semantics  */
	template< typename InputRange >
	auto append(InputRange && source)
	->  iterator_t<InputRange>                { return _doAppend(_getSize(source, 0), source); }
	//! Same as `std::vector::insert(end(), il)`
	void append(std::initializer_list<T> il)  { append<>(il); }
	//! Same as `std::vector::insert(end(), count, val)`
	void append(size_type count, const T & val);

	/**
	* @brief Added elements are default initialized, meaning non-class T produces indeterminate values
	* @throw bad_alloc if n > Capacity  */
	void     resize_for_overwrite(size_type n)   { _doResize<_detail::UninitDefaultConstructA>(n); }
	//! Throws bad_alloc if n > Capacity. (Value-initializes added elements, same as std::vector::resize)
	void     resize(size_type n)                 { _doResize<_detail::UninitFillA>(n); }

	template< typename ForwardRange >
	iterator insert_range(const_iterator pos, ForwardRange && source) &;

	iterator insert(const_iterator pos, T && val) &       { return emplace(pos, std::move(val)); }
	iterator insert(const_iterator pos, const T & val) &  { return emplace(pos, val); }

	template< typename... Args >
	iterator emplace(const_iterator pos, Args &&... elemInitArgs) &;  //!< Throws bad_alloc when full

	template< typename... Args >
	T &      emplace_back(Args &&... args) &;  //!< Throws bad_alloc when full

	//! Throws bad_alloc when full
	void     push_back(T && val)       { emplace_back(std::move(val)); }
	//! Throws bad_alloc when full
	void     push_back(const T & val)  { emplace_back(val); }

	void     pop_back() noexcept
		{
			OEL_ASSERT(0 < _size);
			--_size;
			data()[_size].~T();
		}

	iterator unordered_erase(iterator pos) &   { _eraseUnorder(pos);  return pos; }

	iterator erase(iterator pos) &             { _erase(pos);  return pos; }

	iterator erase(iterator first, const_iterator last) &;
	//! Equivalent to erase(first, end()) (but potentially faster), making first the new end
	void     erase_to_end(iterator first) noexcept
		{
			T *const newEnd = to_pointer_contiguous(first);
			OEL_ASSERT(data() <= newEnd and newEnd <= data() + _size);
			_detail::Destroy(newEnd, data() + _size);
			_size = newEnd - data();
		}

	void      clear() noexcept        { erase_to_end(begin()); }

	bool      full() const noexcept   { return Capacity == _size; }

	size_type size() const noexcept   OEL_ALWAYS_INLINE { return _size; }

	static constexpr size_type capacity() noexcept { return Capacity; }
	static constexpr size_type max_size() noexcept { return Capacity; }

	iterator       begin() noexcept         OEL_ALWAYS_INLINE { return _makeIterator(this->_data); }
	const_iterator begin() const noexcept   OEL_ALWAYS_INLINE { return _makeConstIter(this->_data); }

	iterator       end() noexcept         OEL_ALWAYS_INLINE { return _makeIterator(this->_data + _size); }
	const_iterator end() const noexcept   OEL_ALWAYS_INLINE { return _makeConstIter(this->_data + _size); }

	// T *       data() noexcept
	// const T * data() const noexcept
	using _base::data;

	T &       operator[](size_t index) noexcept
		{
			OEL_ASSERT(index < static_cast<size_t>(_size));
			return data()[index];
		}
	const T & operator[](size_t index) const noexcept
		{
			OEL_ASSERT(index < static_cast<size_t>(_size));
			return data()[index];
		}



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
	using _base::_size;

	iterator _makeIterator(storage_for<T> * pos)
	{
		return _detail::ArrayIteratorMaker<iterator>
		{	reinterpret_cast<T *>(pos),
			reinterpret_cast<const _detail::InplaceGrowarrProxy<T, Size> *>(this)
		};
	}
	const_iterator _makeConstIter(const storage_for<T> * pos) const
	{
		return _detail::ArrayIteratorMaker<const_iterator>
		{	reinterpret_cast<const T *>(pos),
			reinterpret_cast<const _detail::InplaceGrowarrProxy<T, Size> *>(this)
		};
	}


	template< typename Range > // pass dummy int to prefer this overload
	static auto _getSize(Range & r, int) -> decltype((size_t) oel::ssize(r)) { return oel::ssize(r); }

	template< typename Range >
	static false_type _getSize(Range &, long) { return {}; }


	size_type _unusedCapacity() const
	{
		return Capacity - _size;
	}

#ifdef __GNUC__
	#pragma GCC diagnostic push
	#if __GNUC__ >= 8
	#pragma GCC diagnostic ignored "-Wclass-memaccess"
	#endif
#endif

	void _eraseUnorder(iterator const pos)
	{
		if constexpr (is_trivially_relocatable<T>::value)
		{
			T *const ptr = std::addressof(*pos);
			ptr-> ~T();
			--_size;
			auto mem = reinterpret_cast<storage_for<T> *>(ptr);
			*mem = this->_data[_size];  // relocate last element to pos
		}
		else
		{	*pos = std::move(this->back());
			pop_back();
		}
	}

	void _erase(iterator const pos)
	{
		if constexpr (is_trivially_relocatable<T>::value)
		{
			T *const ptr = to_pointer_contiguous(pos);
			OEL_ASSERT(data() <= ptr and ptr < data() + _size);

			ptr-> ~T();
			T *const next = ptr + 1;
			size_t const nAfter = _size - (next - data());
			std::memmove(ptr, next, sizeof(T) * nAfter); // move [pos + 1, _end) to [pos, _end - 1)
			--_size;
		}
		else
		{	iterator last = std::move(pos + 1, end(), pos);
			(*last).~T();
			--_size;
		}
	}


	template< typename UninitFiller >
	void _doResize(size_type const newSize)
	{
		if (Capacity < newSize)
			_detail::BadAlloc::raise();

		T *const oldEnd = data() + _size;
		T *const newEnd = data() + newSize;
		if (oldEnd < newEnd)
			UninitFiller::call(oldEnd, newEnd);
		else
			_detail::Destroy(newEnd, oldEnd);

		_size = newSize;
	}


	template< typename SizedRange >
	iterator_t<SizedRange> _assign(size_t count, SizedRange & src)
	{
		if (Capacity >= count)
			return this->doAssign(adl_begin(src), count);

		_detail::BadAlloc::raise();
	}

	template< typename InputRange >
	iterator_t<InputRange> _assign(false_type, InputRange & src)
	{	// no fast way of getting size
		clear();
		return append(src);
	}

	template< typename SizedRange >
	iterator_t<SizedRange> _doAppend(size_t count, SizedRange & src)
	{
		if (_unusedCapacity() >= count)
		{
			auto first = adl_begin(src);
			first = _detail::UninitCopy(first, count, data() + _size);
			_size += count;

			return first;
		}
		_detail::BadAlloc::raise();
	}

	template< typename InputRange >
	iterator_t<InputRange> _doAppend(false_type, InputRange & src)
	{	// number of items unknown (slowest)
		auto f = adl_begin(src);
		auto l = adl_end(src);
		for (; f != l; ++f)
			emplace_back(*f);

		return f;
	}
};

template< typename T, size_t Capacity, typename Size >
void inplace_growarr<T, Capacity, Size>::append(size_type n, const T & val)
{
	if (_unusedCapacity() < n)
		_detail::BadAlloc::raise();

	size_type newSize = _size + n;
	_detail::UninitFillA::call(data() + _size, data() + newSize, val);
	_size = newSize;
}

template< typename T, size_t Capacity, typename Size> template<typename... Args >
T & inplace_growarr<T, Capacity, Size>::emplace_back(Args &&... args) &
{
	if (Capacity > _size)
	{
		T *const pos = data() + _size;
		::new(static_cast<void *>(pos)) T(static_cast<Args &&>(args)...);
		++_size;
		return *pos;
	}
	_detail::BadAlloc::raise();
}

template< typename T, size_t Capacity, typename Size> template<typename... Args >
typename inplace_growarr<T, Capacity, Size>::iterator  inplace_growarr<T, Capacity, Size>::
	emplace(const_iterator pos, Args &&... args) &
{
	(void) _detail::AssertTrivialRelocate<T>{};
	OEL_ASSERT(begin() <= pos and pos <= end());

	if (Capacity > _size)
	{
		auto const pPos = const_cast<T *>(to_pointer_contiguous(pos));
		size_t const nAfterPos = _size - (pPos - data());
		// Temporary in case constructor throws or source is an element of this array at pos or after
		storage_for<T> tmp;
		::new(static_cast<void *>(&tmp)) T(static_cast<Args &&>(args)...);
		// Relocate [pos, end) to [pos + 1, end + 1), leaving memory at pos uninitialized (conceptually)
		std::memmove(pPos + 1, pPos, sizeof(T) * nAfterPos);
		++_size;

		std::memcpy(pPos, &tmp, sizeof(T)); // relocate the new element to pos

		return _makeIterator(reinterpret_cast<storage_for<T> *>(pPos));
	}
	_detail::BadAlloc::raise();
}

template< typename T, size_t Capacity, typename Size> template<typename ForwardRange >
typename inplace_growarr<T, Capacity, Size>::iterator  inplace_growarr<T, Capacity, Size>::
	insert_range(const_iterator pos, ForwardRange && src) &
{
	(void) _detail::AssertTrivialRelocate<T>{};
	OEL_ASSERT(begin() <= pos and pos <= end());

	auto first = adl_begin(src);
	auto const count = _detail::CountOrEnd(src);

	static_assert( std::is_same_v<decltype(count), size_t const>,
			"insert_range requires that source models std::ranges::forward_range or that source.size() is valid" );

	if (_unusedCapacity() >= count)
	{
		auto const pPos = const_cast<T *>(to_pointer_contiguous(pos));
		size_t const bytesAfterPos = sizeof(T) * ((data() + _size) - pPos);
		T *const dLast = pPos + count;
		// Relocate elements to make space, leaving [pos, pos + count) uninitialized (conceptually)
		std::memmove(dLast, pPos, bytesAfterPos);
		_size += count;
		// Construct new
		if constexpr (can_memmove_with<T *, decltype(first)>)
		{
			_detail::MemcpyCheck(first, count, pPos);
		}
		else
		{	T * dest = pPos;
			OEL_TRY_
			{
				while (dest != dLast)
				{
					::new(static_cast<void *>(dest)) T(*first);
					++first; ++dest;
				}
			}
			OEL_CATCH_ALL
			{	// relocate back to fill hole
				std::memmove(dest, dLast, bytesAfterPos);
				_size -= (dLast - dest);
				OEL_RETHROW;
			}
		}

		return _makeIterator(reinterpret_cast<storage_for<T> *>(pPos));
	}
	_detail::BadAlloc::raise();
}


template< typename T, size_t Capacity, typename Size >
typename inplace_growarr<T, Capacity, Size>::iterator  inplace_growarr<T, Capacity, Size>::
	erase(iterator first, const_iterator last) &
{
	(void) _detail::AssertTrivialRelocate<T>{};

	T *const      pFirst = to_pointer_contiguous(first);
	const T *const pLast = to_pointer_contiguous(last);
	OEL_ASSERT(data() <= pFirst and pFirst <= pLast and last <= end());

	difference_type nErase = pLast - pFirst;
	if (0 < nErase)
	{
		_detail::Destroy(pFirst, pLast);
		size_t const nAfterLast = _size - (pLast - data());
		// move [last, _end) to [first, first + nAfterLast)
		std::memmove(pFirst, pLast, sizeof(T) * nAfterLast);
		_size -= nErase;
	}
	return first;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

} // namespace oel
