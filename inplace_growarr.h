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

template< typename Iterator >
struct inplace_insert_range_return
{
	Iterator in;
};


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


//! inplace_growarr is trivially relocatable if T is
template< typename T, size_t C, typename S >
is_trivially_relocatable<T> specify_trivial_relocate(inplace_growarr<T, C, S>);


//! Resizable array, statically allocated. Specify maximum size as template argument.
/**
* In general, only that which differs from std::inplace_vector (C++26) is documented.
*
* A few functions require that T is trivially relocatable (see oel::is_trivially_relocatable):
* emplace, insert, insert_range
*
* For any function which takes a range, `end(range)` is not needed if `range.size()` is valid.
*/
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

	//! Default-initializes elements, can be significantly faster if T is scalar or has trivial default constructor
	/** @copydetails resize_for_overwrite(size_type)  */
	inplace_growarr(size_type size, for_overwrite_t)
		{
			if (Capacity < size)
				_detail::BadAlloc::raise();

			_detail::UninitDefaultConstructA::call(data(), data() + size);
			_size = size;
		}
	//! (Value-initializes elements, same as standard containers)
	explicit inplace_growarr(size_type size)
		{
			if (Capacity < size)
				_detail::BadAlloc::raise();

			_detail::UninitFillA::call(data(), data() + size);
			_size = size;
		}

	template< typename InputRange >
	inplace_growarr(from_range_t, InputRange && r)   { append(r); }

	inplace_growarr(std::initializer_list<T> il)     { append(il); }

	inplace_growarr(inplace_growarr && other) noexcept       { _relocateFrom(other); }
	explicit inplace_growarr(const inplace_growarr & other)  { append(other); }

	~inplace_growarr()   { _detail::Destroy(data(), data() + _size); }

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

	//! Like std::inplace_vector::assign_range, but stops when full instead of throwing bad_alloc
	/** @return An iterator pointing to the first element of source that was not inserted,
	*	or equal to `end(source)` if no such element exists
	*
	* Any elements held before the call are either assigned to or destroyed. */
	template< typename InputRange >
	auto try_assign(InputRange && source) -> borrowed_iterator_t<InputRange>;

	//! Almost same as std::inplace_vector::assign_range
	/** @return Iterator `begin(source)` incremented by the number of elements in source
	* @copydetails try_assign(InputRange &&)  */
	template< typename InputRange >
	auto assign(InputRange && source) -> borrowed_iterator_t<InputRange>;

	void assign(size_type count, const T & val)   { clear();  append(count, val); }

	//! Equivalent to std::inplace_vector::try_append_range
	/**
	* A previous end iterator will point to the first element added, after the call. */
	template< typename InputRange >
	auto try_append(InputRange && source) -> borrowed_iterator_t<InputRange>;

	//! Almost same as std::inplace_vector::append_range
	/** @return Iterator `begin(source)` incremented by the number of elements in source
	* @copydetails try_append(InputRange &&)  */
	template< typename InputRange >
	auto append(InputRange && source) -> borrowed_iterator_t<InputRange>;
	//! Equivalent to `std::inplace_vector::insert(end(), count, val)`
	void append(size_type count, const T & val);

	//! Default-initializes added elements, can be significantly faster if T is scalar or trivially constructible
	/**
	* Objects of scalar type get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void resize_for_overwrite(size_type n)   { _doResize<_detail::UninitDefaultConstructA>(n); }
	void resize(size_type n)                 { _doResize<_detail::UninitFillA>(n); }

	//! Similar to std::inplace_vector::insert_range
	/** @return Struct with `in` variable which is `begin(source)` incremented by the number of elements in source
	* @param source must model std::ranges::forward_range or `source.size()` must be valid
	*
	* After the call, pos points at the first element inserted. */
	template< typename Range >
	auto insert_range(const_iterator pos, Range && source) -> inplace_insert_range_return< borrowed_iterator_t<Range> >;

	iterator insert(const_iterator pos, T && val) &       { return emplace(pos, std::move(val)); }
	iterator insert(const_iterator pos, const T & val) &  { return emplace(pos, val); }

	template< typename... Args >
	iterator emplace(const_iterator pos, Args &&... args) &;

	template< typename... Args >
	T &      emplace_back(Args &&... args) &
		{
			if (Capacity != _size)
				return unchecked_emplace_back(static_cast<Args &&>(args)...);
			else
				_detail::BadAlloc::raise();
		}

	void push_back(T && val)       { emplace_back(std::move(val)); }
	void push_back(const T & val)  { emplace_back(val); }

	template< typename... Args >
	T &  unchecked_emplace_back(Args &&... args) &;

	void unchecked_push_back(T && val)       { unchecked_emplace_back(std::move(val)); }
	void unchecked_push_back(const T & val)  { unchecked_emplace_back(val); }

	void pop_back() noexcept
		{
			OEL_ASSERT(_size > 0);
			--_size;
			data()[_size].~T();
		}

	//! Erase the element at pos without maintaining order of elements after pos
	/**
	* The iterator pos still corresponds to the same index in the sequence after the call.
	* If pos pointed to the back element, it will be equal to end.
	* Constant complexity (compared to linear in the distance between pos and `end()` for normal erase). */
	void     unordered_erase(iterator pos);

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
	//! Equivalent to `erase(first, end())`, but potentially faster and does not require assignable T
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


	template< typename InputIter >
	InputIter _doAssign(InputIter src, size_type const count)
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
					++dest; ++src_;
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

	template< typename InputRange >
	auto _tryEmplBackRange(InputRange & src)
	{
		auto it = adl_begin(src);
		auto l  = adl_end(src);
		for (; it != l and _size != Capacity; ++it)
			unchecked_emplace_back(*it);

		return it;
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
};

template< typename T, size_t Capacity, typename Size >
template< typename InputRange >
auto inplace_growarr<T, Capacity, Size>::try_assign(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsSized<InputRange>)
	{
		auto it = adl_begin(source);

		auto n = _detail::Size(source);
		if (as_unsigned(n) > Capacity)
			n = Capacity;

		return _doAssign(std::move(it), static_cast<size_type>(n));
	}
	else
	{	clear();
		return _tryEmplBackRange(source);
	}
}

template< typename T, size_t Capacity, typename Size >
template< typename InputRange >
auto inplace_growarr<T, Capacity, Size>::assign(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsSized<InputRange>)
	{
		auto      it = adl_begin(source);
		auto const n = _detail::Size(source);

		if (as_unsigned(n) > Capacity)
			_detail::BadAlloc::raise();

		return _doAssign(std::move(it), static_cast<size_type>(n));
	}
	else
	{	clear();
		return _emplBackRange(source);
	}
}

template< typename T, size_t Capacity, typename Size >
template< typename InputRange >
inline auto inplace_growarr<T, Capacity, Size>::try_append(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsSized<InputRange>)
	{
		auto it = adl_begin(source);

		auto const n     = as_unsigned(_detail::Size(source));
		auto const spare = as_unsigned(oel::spare_capacity(*this));
		auto const min   = n < spare ? n : spare;

		it = _detail::UninitCopy(std::move(it), min, data() + _size);
		_size += static_cast<Size>(min);

		return it;
	}
	else
	{	return _tryEmplBackRange(source);
	}
}

template< typename T, size_t Capacity, typename Size >
template< typename InputRange >
inline auto inplace_growarr<T, Capacity, Size>::append(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsSized<InputRange>)
	{
		auto      it = adl_begin(source);
		auto const n = as_unsigned(_detail::Size(source));

		if (as_unsigned(oel::spare_capacity(*this)) < n)
			_detail::BadAlloc::raise();

		it = _detail::UninitCopy(std::move(it), n, data() + _size);
		_size += static_cast<Size>(n);

		return it;
	}
	else
	{	auto const oldSize = _size;
		OEL_TRY_
		{
			return _emplBackRange(source);
		}	// Catch with cleanup needed only when called from constructor
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
	if (oel::spare_capacity(*this) < count)
		_detail::BadAlloc::raise();

	size_type newSize = _size + count;
	_detail::UninitFillA::call(data() + _size, data() + newSize, val);
	_size = newSize;
}


template< typename T, size_t Capacity, typename Size >
template< typename Range >
auto inplace_growarr<T, Capacity, Size>::insert_range(const_iterator pos, Range && source)
->	inplace_insert_range_return< borrowed_iterator_t<Range> >
{
	(void) _detail::AssertTrivialRelocate<T>{};
	(void) _detail::AssertForwardOrSizedRange<Range>{};
	OEL_ASSERT(begin() <= pos and pos <= end());

	auto       srcIt = adl_begin(source);
	auto const n = _detail::UDist(source);

	if (as_unsigned(oel::spare_capacity(*this)) < n)
		_detail::BadAlloc::raise();

	auto mutPos = const_cast<T *>(pos);
	size_t const bytesAfterPos = sizeof(T) * ((data() + _size) - mutPos);
	T *const dLast = mutPos + n;
	// Relocate elements to make space, leaving [pos, pos + n) uninitialized (conceptually)
	std::memmove(static_cast<void *>(dLast), static_cast<const void *>(mutPos), bytesAfterPos);
	_size += static_cast<Size>(n);
	// Construct new
	if constexpr (can_memmove_with<T *, decltype(srcIt)>)
	{
		_detail::MemcpyCheck(srcIt, n, mutPos);
		srcIt += n;
	}
	else
	{	OEL_TRY_
		{
			while (mutPos != dLast)
			{
				::new(static_cast<void *>(mutPos)) T(*srcIt);
				++mutPos; ++srcIt;
			}
		}
		OEL_CATCH_ALL
		{	// relocate back to fill hole
			std::memmove(static_cast<void *>(mutPos), static_cast<const void *>(dLast), bytesAfterPos);
			_size -= static_cast<Size>(dLast - mutPos);
			OEL_RETHROW;
		}
	}

	return {std::move(srcIt)};
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
void inplace_growarr<T, Capacity, Size>::unordered_erase(iterator pos)
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
}

} // namespace oel
