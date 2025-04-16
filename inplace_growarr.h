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

template< size_t Cap, typename SizeT >
struct _toInplaceGrowarrFn
{
	template< typename InputRange >
	friend auto operator |(InputRange && r, _toInplaceGrowarrFn)
		{
			using T = iter_value_t< iterator_t<InputRange> >;
			return inplace_growarr<T, Cap, SizeT>(from_range, static_cast<InputRange &&>(r));
		}

	template< typename InputRange >
	auto operator()(InputRange && r) const   { return static_cast<InputRange &&>(r) | *this; }
};
//! `r | to_inplace_growarr<C>` is same as `r | std::ranges::to< inplace_growarr<T, C> >()` with T deduced from `r`
/**
* Regular function syntax can also be used: `to_inplace_growarr<30>(r)` */
template< size_t Cap, typename SizeT = size_t >
inline constexpr _toInplaceGrowarrFn<Cap, SizeT> to_inplace_growarr;


//! inplace_growarr is trivially relocatable if T is
template< typename T, size_t C, typename S >
is_trivially_relocatable<T> specify_trivial_relocate(inplace_growarr<T, C, S>);


//! Resizable array, statically allocated. Specify maximum size as template argument.
/**
* The type of the internal size variable can also be specified with the last, optional template argument.
*
* In general, only that which differs from std::inplace_vector (C++26) is documented.
*
* A few functions require that T is trivially relocatable (see oel::is_trivially_relocatable):
* emplace, insert, try_insert
*
* For any function which takes a range, `end(range)` is not needed if `range.size()` is valid.
*/
template< typename T, size_t Cap, typename SizeT >
class inplace_growarr
 :	public _arrayInterface< inplace_growarr<T, Cap, SizeT> >
{
	static_assert(Cap <= std::numeric_limits<SizeT>::max(), "Capacity does not fit in SizeT");

public:
	using value_type      = T;
	using size_type       = SizeT;
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
			if (Cap < size)
				_detail::BadAlloc::raise();

			_detail::UninitDefaultConstructA::call(data(), data() + size);
			_size = size;
		}
	//! (Value-initializes elements, same as standard containers)
	explicit inplace_growarr(size_type size)
		{
			if (Cap < size)
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

	//! Like std::inplace_vector::try_append_range, but replaces old contents
	/** @return An iterator pointing to the first element of source that was not inserted,
	*	or equal to `end(source)` if no such element exists
	*
	* Any elements held before the call are either assigned to or destroyed. */
	template< typename InputRange >
	auto try_assign(InputRange && source) -> borrowed_iterator_t<InputRange>;

	void assign(size_type count, const T & val)
		{
			clear();
			if (!try_append(count, val))
				_detail::BadAlloc::raise();
		}

	//! Almost same as std::inplace_vector::append_range
	/** @return Iterator `begin(source)` incremented by the number of elements in source
	* @copydetails try_append(InputRange &&)  */
	template< typename InputRange >
	auto append(InputRange && source) -> borrowed_iterator_t<InputRange>;

	//! Equivalent to std::inplace_vector::try_append_range
	/**
	* A previous end iterator will point to the first element added, after the call. */
	template< typename InputRange >
	auto try_append(InputRange && source) -> borrowed_iterator_t<InputRange>;

	//! Like `std::inplace_vector::insert(end(), count, val)`, but returns false instead of throwing bad_alloc
	/** @return `count <= spare_capacity(*this)` which indicates success
	*
	* There are no effects if spare capacity is too small. */
	bool try_append(size_type count, const T & val);

	//! Default-initializes added elements, can be significantly faster if T is scalar or trivially constructible
	/**
	* Objects of scalar type get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void resize_for_overwrite(size_type n)   { _doResize<_detail::UninitDefaultConstructA>(n); }
	void resize(size_type n)                 { _doResize<_detail::UninitFillA>(n); }

	//! Like std::inplace_vector::insert_range, but does not throw bad_alloc if spare capacity is too small
	/** @return Iterator `begin(source)` incremented by `n` if `n <= spare_capacity(*this)`, where
	*	`n` is the number of elements in source. Else, on failure, returns `begin(source)` unchanged
	* @param source must model std::ranges::forward_range or `source.size()` must be valid
	*
	* After the call, pos points at the first element inserted.
	* If spare capacity is too small, there are no effects on this container. */
	template< typename Range >
	auto try_insert(const_iterator pos, Range && source) -> borrowed_iterator_t<Range>;

	iterator insert(const_iterator pos, T && val) &       { return emplace(pos, std::move(val)); }
	iterator insert(const_iterator pos, const T & val) &  { return emplace(pos, val); }

	template< typename... Args >
	iterator emplace(const_iterator pos, Args &&... args) &;

	template< typename... Args >
	T &      emplace_back(Args &&... args) &
		{
			if (Cap != _size)
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

	bool      full() const noexcept   { return Cap == _size; }

	OEL_ALWAYS_INLINE
	size_type size() const noexcept   { return _size; }

	static constexpr size_type capacity() noexcept { return Cap; }
	static constexpr size_type max_size() noexcept { return Cap; }

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
	SizeT          _size{};
	storage_for<T> _data[Cap];


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
		if (Cap < newSize)
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
		for (; it != l and _size != Cap; ++it)
			unchecked_emplace_back(*it);

		return it;
	}
};

template< typename T, size_t Cap, typename SizeT >
template< typename InputRange >
auto inplace_growarr<T, Cap, SizeT>::try_assign(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsSized<InputRange>)
	{
		auto it = adl_begin(source);

		auto n = _detail::Size(source);
		if (as_unsigned(n) > Cap)
			n = Cap;

		return _doAssign(std::move(it), static_cast<size_type>(n));
	}
	else
	{	clear();
		return _tryEmplBackRange(source);
	}
}

template< typename T, size_t Cap, typename SizeT >
template< typename InputRange >
inline auto inplace_growarr<T, Cap, SizeT>::try_append(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsSized<InputRange>)
	{
		auto it = adl_begin(source);

		auto const n     = as_unsigned(_detail::Size(source));
		auto const spare = as_unsigned(oel::spare_capacity(*this));
		auto const min   = n < spare ? n : spare;

		it = _detail::UninitCopy(std::move(it), min, data() + _size);
		_size += static_cast<SizeT>(min);

		return it;
	}
	else
	{	return _tryEmplBackRange(source);
	}
}

template< typename T, size_t Cap, typename SizeT >
template< typename InputRange >
inline auto inplace_growarr<T, Cap, SizeT>::append(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsSized<InputRange>)
	{
		auto      it = adl_begin(source);
		auto const n = as_unsigned(_detail::Size(source));

		if (as_unsigned(oel::spare_capacity(*this)) < n)
			_detail::BadAlloc::raise();

		it = _detail::UninitCopy(std::move(it), n, data() + _size);
		_size += static_cast<SizeT>(n);

		return it;
	}
	else
	{	auto const oldSize = _size;
		OEL_TRY_
		{
			auto it = adl_begin(source);
			auto l  = adl_end(source);
			for (; it != l; ++it)
				emplace_back(*it);

			return it;
		}	// Catch with cleanup needed only when called from constructor
		OEL_CATCH_ALL
		{
			erase_to_end(begin() + oldSize);
			OEL_RETHROW;
		}
	}
}

template< typename T, size_t Cap, typename SizeT >
bool inplace_growarr<T, Cap, SizeT>::try_append(size_type count, const T & val)
{
	if (oel::spare_capacity(*this) >= count)
	{
		size_type const newSize{_size + count};
		_detail::UninitFillA::call(data() + _size, data() + newSize, val);
		_size = newSize;

		return true;
	}
	else
	{	return false;
	}
}


template< typename T, size_t Cap, typename SizeT >
template< typename Range >
auto inplace_growarr<T, Cap, SizeT>::try_insert(const_iterator pos, Range && source)
->	borrowed_iterator_t<Range>
{
	(void) _detail::AssertTrivialRelocate<T>{};
	(void) _detail::AssertForwardOrSizedRange<Range>{};
	OEL_ASSERT(begin() <= pos and pos <= end());

	auto       srcIt = adl_begin(source);
	auto const n = _detail::UDist(source);

	if (as_unsigned(oel::spare_capacity(*this)) >= n)
	{
		auto mutPos = const_cast<T *>(pos);
		size_t const bytesAfterPos = sizeof(T) * ((data() + _size) - mutPos);
		T *const dLast = mutPos + n;
		// Relocate elements to make space, leaving [pos, pos + n) uninitialized (conceptually)
		std::memmove(static_cast<void *>(dLast), static_cast<const void *>(mutPos), bytesAfterPos);
		_size += static_cast<SizeT>(n);
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
				_size -= static_cast<SizeT>(dLast - mutPos);
				OEL_RETHROW;
			}
		}
	}
	return srcIt;
}

template< typename T, size_t Cap, typename SizeT >
template< typename... Args >
typename inplace_growarr<T, Cap, SizeT>::iterator
	inplace_growarr<T, Cap, SizeT>::emplace(const_iterator pos, Args &&... args) &
{
	(void) _detail::AssertTrivialRelocate<T>{};
	OEL_ASSERT(begin() <= pos and pos <= end());

	if (Cap != _size)
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

template< typename T, size_t Cap, typename SizeT >
template< typename... Args >
inline T & inplace_growarr<T, Cap, SizeT>::unchecked_emplace_back(Args &&... args) &
{
	OEL_ASSERT(_size < Cap);

	T *const pos = data() + _size;
	::new(static_cast<void *>(pos)) T(static_cast<Args &&>(args)...);
	++_size;
	return *pos;
}


template< typename T, size_t Cap, typename SizeT >
inplace_growarr<T, Cap, SizeT> &  inplace_growarr<T, Cap, SizeT>::operator =(inplace_growarr && other) &
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


template< typename T, size_t Cap, typename SizeT >
void inplace_growarr<T, Cap, SizeT>::unordered_erase(iterator pos)
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
