#pragma once

// Copyright 2016 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/array_interface.h"
#include "auxi/inplace_growarr_detail.h"
#include "optimize_ext/default.h"

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
 :	public _arrayInterface< inplace_growarr<T, Capacity, Size> >
{
	static_assert(Capacity <= std::numeric_limits<Size>::max(), "Capacity does not fit in type Size");

public:
	using value_type      = T;
	using size_type       = Size;
	using difference_type = ptrdiff_t;

	using iterator               = T *;
	using const_iterator         = const T *;
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
	inplace_growarr(from_range_t, InputRange && r)   { append(r); }

	inplace_growarr(std::initializer_list<T> il)     { append(il); }

	inplace_growarr(inplace_growarr && other) noexcept  { _relocateFrom(other); }
	explicit inplace_growarr(const inplace_growarr & other)
		{
			_size = other._size;
			_detail::UninitCopy(other.data(), other._size, data());
		}

	~inplace_growarr() noexcept   { _detail::Destroy(data(), data() + _size); }

	inplace_growarr & operator =(inplace_growarr && other) &
		noexcept(std::is_nothrow_move_assignable_v<T> or is_trivially_relocatable<T>::value);
	inplace_growarr & operator =(const inplace_growarr & other) &
		{
			if (this != &other)
				_doAssign(other.data(), other._size);

			return *this;
		}
	inplace_growarr & operator =(const inplace_growarr &&) = delete;

	inplace_growarr & operator =(std::initializer_list<T> il) &  { assign(il);  return *this; }

	/**
	* @brief Replace the contents with source
	* @throw bad_alloc if count > Capacity  */
	template< typename InputRange >
	auto assign(InputRange && source) -> borrowed_iterator_t<InputRange>;

	void assign(size_type count, const T & val)   { clear();  append(count, val); }

	template< typename InputRange >
	bool try_assign(InputRange && source)         { return false; } // TODO

	/**
	* @brief Add at end the elements from range (in order)
	* @param source object which begin and end can be called on (an array, STL container or iterator_range)
	* @return begin(source) incremented to end of source
	*
	* Any previous end iterator will point to the first element added.
	* Strong exception safety, aka. commit or rollback semantics  */
	template< typename InputRange >
	auto append(InputRange && source) -> borrowed_iterator_t<InputRange>;
	//! Same as `std::vector::insert(end(), il)`
	void append(std::initializer_list<T> il)  { append<>(il); }
	//! Same as `std::vector::insert(end(), count, val)`
	void append(size_type count, const T & val);

	template< typename InputRange >
	bool try_append(InputRange && source)     { return false; } // TODO

	/**
	* @brief Added elements are default initialized, meaning non-class T produces indeterminate values
	* @throw bad_alloc if n > Capacity  */
	void     resize_for_overwrite(size_type n)   { _doResize<_detail::UninitDefaultConstructA>(n); }
	//! Throws bad_alloc if n > Capacity. (Value-initializes added elements, same as std::vector::resize)
	void     resize(size_type n)                 { _doResize<_detail::UninitFillA>(n); }

	template< typename Range >
	iterator insert_range(const_iterator pos, Range && source) &;

	template< typename Range >
	bool     try_insert_range(const_iterator pos, Range && source)  { return false; } // TODO

	iterator insert(const_iterator pos, T && val) &       { return emplace(pos, std::move(val)); }
	iterator insert(const_iterator pos, const T & val) &  { return emplace(pos, val); }

	template< typename... Args >
	iterator emplace(const_iterator pos, Args &&... elemInitArgs) &;  //!< Throws bad_alloc when full

	//! Throws bad_alloc when full
	template< typename... Args >
	T &      emplace_back(Args &&... args) &
		{
			if (Capacity != _size)
				return unchecked_emplace_back(static_cast<Args &&>(args)...);
			else
				_detail::BadAlloc::raise();
		}
	//! Throws bad_alloc when full
	void     push_back(T && val)       { emplace_back(std::move(val)); }
	//! Throws bad_alloc when full
	void     push_back(const T & val)  { emplace_back(val); }

	//! Undefined behavior if full
	template< typename... Args >
	T &      unchecked_emplace_back(Args &&... args) &;

	//! Undefined behavior if full
	void     unchecked_push_back(T && val)       { unchecked_emplace_back(std::move(val)); }
	//! Undefined behavior if full
	void     unchecked_push_back(const T & val)  { unchecked_emplace_back(val); }

	void     pop_back() noexcept
		{
			OEL_ASSERT(_size > 0);
			--_size;
			data()[_size].~T();
		}

	iterator unordered_erase(iterator pos) &;

	iterator erase(iterator pos) &
		{
			_size = _detail::Erase(_size, data(), pos);
			return pos;
		}
	iterator erase(iterator first, const_iterator last) &
		{
			_size = _detail::Erase(_size, data(), first, last);
			return first;
		}
	//! Equivalent to erase(first, end()) (but potentially faster), making first the new end
	void     erase_to_end(iterator first) noexcept
		{
			OEL_ASSERT(begin() <= first and first <= end());
			_detail::Destroy(first, data() + _size);
			_size = first - data();
		}

	void      clear() noexcept        { erase_to_end(begin()); }

	bool      full() const noexcept   { return Capacity == _size; }

	OEL_ALWAYS_INLINE
	size_type size() const noexcept   { return _size; }

	static constexpr size_type capacity() noexcept { return Capacity; }
	static constexpr size_type max_size() noexcept { return Capacity; }

	OEL_ALWAYS_INLINE
	iterator       begin() noexcept         { return data(); }
	OEL_ALWAYS_INLINE
	const_iterator begin() const noexcept   { return data(); }

	iterator       end() noexcept         { return data() + _size; }
	const_iterator end() const noexcept   { return data() + _size; }

	T *       data() noexcept         { return reinterpret_cast<T *>(_data); }
	const T * data() const noexcept   { return reinterpret_cast<const T *>(_data); }

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
	Size           _size{};
	storage_for<T> _data[Capacity];


	void _relocateFrom(inplace_growarr & other) noexcept
	{
		_detail::Relocate(other.data(), other._size, data());
		_size = other._size;
		if constexpr (is_trivially_relocatable<T>::value and !std::is_trivially_copyable_v<T>)
			other._size = 0;
	}


	size_type _spareCapacity() const
	{
		return Capacity - _size;
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


	template< typename InputRange >
	auto _emplBackRange(InputRange & src)
	{
		auto it = adl_begin(src);
		auto l  = adl_end(src);
		for (; it != l; ++it)
			emplace_back(*it);

		return it;
	}

	template< typename InputIter >
	InputIter _doAssign(InputIter src, Size const count)
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

template< typename T, size_t Capacity, typename Size>
template< typename InputRange >
auto inplace_growarr<T, Capacity, Size>::assign(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsSized<InputRange>)
	{
		auto const n = _detail::Size(source);
		if (Capacity >= n)
			return _doAssign(adl_begin(source), n);

		_detail::BadAlloc::raise();
	}
	else
	{	clear();
		return _emplBackRange(source);
	}
}

template< typename T, size_t Capacity, typename Size>
template< typename InputRange >
inline auto inplace_growarr<T, Capacity, Size>::append(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsSized<InputRange>)
	{
		auto const n = as_unsigned(_detail::Size(source));
		if (_spareCapacity() >= n)
		{
			auto first = adl_begin(source);
			first = _detail::UninitCopy(first, n, data() + _size);
			_size += static_cast<Size>(n);

			return first;
		}
		_detail::BadAlloc::raise();
	}
	else
	{	auto const oldSize = _size;
		OEL_TRY_
		{
			return _emplBackRange(source);
		}
		OEL_CATCH_ALL
		{
			erase_to_end(begin() + oldSize);
			OEL_RETHROW;
		}
	}
}

template< typename T, size_t Capacity, typename Size >
void inplace_growarr<T, Capacity, Size>::append(size_type count, const T & val)
{
	if (_spareCapacity() < count)
		_detail::BadAlloc::raise();

	size_type newSize = _size + count;
	_detail::UninitFillA::call(data() + _size, data() + newSize, val);
	_size = newSize;
}

template< typename T, size_t Capacity, typename Size >
template< typename... Args >
inline T & inplace_growarr<T, Capacity, Size>::unchecked_emplace_back(Args &&... args) &
{
	OEL_ASSERT(_size < Capacity);

	T *const pos = data() + _size;
	::new(static_cast<void *>(pos)) T(static_cast<Args &&>(args)...);
	++_size;
	return *pos;
}

template< typename T, size_t Capacity, typename Size >
template< typename... Args >
typename inplace_growarr<T, Capacity, Size>::iterator  inplace_growarr<T, Capacity, Size>::
	emplace(const_iterator pos, Args &&... args) &
{
	(void) _detail::AssertTrivialRelocate<T>{};
	OEL_ASSERT(begin() <= pos and pos <= end());

	if (Capacity != _size)
	{
		auto const mutPos = const_cast<T *>(pos);
		size_t const nAfterPos = _size - (mutPos - data());
		// Temporary in case constructor throws or source is an element of this array at pos or after
		storage_for<T> tmp;
		::new(&tmp) T(static_cast<Args &&>(args)...);
		// Relocate [pos, end) to [pos + 1, end + 1), leaving memory at pos uninitialized (conceptually)
		std::memmove(
			static_cast<void *>(mutPos + 1),
			static_cast<const void *>(mutPos),
			sizeof(T) * nAfterPos );
		++_size;

		std::memcpy(static_cast<void *>(mutPos), &tmp, sizeof(T)); // relocate the new element to pos

		return mutPos;
	}
	_detail::BadAlloc::raise();
}

template< typename T, size_t Capacity, typename Size >
template< typename Range >
typename inplace_growarr<T, Capacity, Size>::iterator  inplace_growarr<T, Capacity, Size>::
	insert_range(const_iterator pos, Range && source) &
{
	(void) _detail::AssertTrivialRelocate<T>{};
	(void) _detail::AssertForwardOrSizedRange<Range>{};
	OEL_ASSERT(begin() <= pos and pos <= end());

	auto       first = adl_begin(source);
	auto const count = _detail::UDist(source);

	if (_spareCapacity() >= count)
	{
		auto const mutPos = const_cast<T *>(pos);
		size_t const bytesAfterPos = sizeof(T) * ((data() + _size) - mutPos);
		T *const dLast = mutPos + count;
		// Relocate elements to make space, leaving [pos, pos + count) uninitialized (conceptually)
		std::memmove(static_cast<void *>(dLast), static_cast<const void *>(mutPos), bytesAfterPos);
		_size += static_cast<Size>(count);
		// Construct new
		if constexpr (can_memmove_with<T *, decltype(first)>)
		{
			_detail::MemcpyCheck(first, count, mutPos);
		}
		else
		{	T * dest = mutPos;
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
				std::memmove(static_cast<void *>(dest), static_cast<const void *>(dLast), bytesAfterPos);
				_size -= (dLast - dest);
				OEL_RETHROW;
			}
		}

		return mutPos;
	}
	_detail::BadAlloc::raise();
}


template< typename T, size_t Capacity, typename Size >
inplace_growarr<T, Capacity, Size> &  inplace_growarr<T, Capacity, Size>::operator =(inplace_growarr && other) &
	noexcept(std::is_nothrow_move_assignable_v<T> or is_trivially_relocatable<T>::value)
{
	if constexpr (is_trivially_relocatable<T>::value)
	{
		_detail::Destroy(data(), data() + _size);
		_relocateFrom(other);
	}
	else
	{	(void) _detail::AssertNothrowMoveConstruct<T>{};
		_doAssign(std::move_iterator{other.data()}, other._size);
	}
	return *this;
}


template< typename T, size_t Capacity, typename Size >
typename inplace_growarr<T, Capacity, Size>::iterator  inplace_growarr<T, Capacity, Size>::unordered_erase(iterator pos) &
{
	if constexpr (is_trivially_relocatable<T>::value)
	{
		T & elem = *pos;
		elem.~T();
		--_size;
		auto mem = reinterpret_cast< storage_for<T> * >(&elem);
		*mem = _data[_size];  // relocate last element to pos
	}
	else
	{	*pos = std::move(this->back());
		pop_back();
	}
	return pos;
}

} // namespace oel
