#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/algo_detail.h"
#include "auxi/array_iterator.h"
#include "optimize_ext/default.h"
#include "align_allocator.h"

#include <algorithm>


/** @file
*/

namespace oel
{

template<typename T, size_t C, typename S>
is_trivially_relocatable<T> specify_trivial_relocate(fixcap_array<T, C, S> &&);

//! Overloads generic erase_unstable(RandomAccessContainer &, RandomAccessContainer::size_type) (in range_algo.h)
template<typename T, size_t C, typename Size>  inline
void erase_unstable(fixcap_array<T, C, Size> & a, Size index)  { a.erase_unstable(a.begin() + index); }

//! @name GenericContainerInsert
//!@{
// Overloads of generic functions for inserting into container (in range_algo.h)
template<typename T, size_t C, typename S, typename InputRange>  inline
void assign(fixcap_array<T, C, S> & dest, const InputRange & source)  { dest.assign(source); }

template<typename T, size_t C, typename S, typename InputRange>  inline
void append(fixcap_array<T, C, S> & dest, const InputRange & source)  { dest.append(source); }

template<typename T, size_t C, typename S>  inline
void append(fixcap_array<T, C, S> & dest, S count, const T & val)  { dest.append(count, val); }

template<typename T, size_t C, typename S, typename ForwardRange>  inline
typename fixcap_array<T, C, S>::iterator
	insert(fixcap_array<T, C, S> & dest, typename fixcap_array<T, C, S>::const_iterator pos, const ForwardRange & source)
	{
		return dest.insert_r(pos, source);
	}
//!@}

/**
* @brief Resizable array, statically allocated. Specify maximum size as template argument.
*
* Behaviour which equals that of std::vector is mostly not documented. */
template<typename T, size_t Capacity, typename Size/* = size_t*/>
class fixcap_array
{
public:
	using value_type      = T;
	using reference       = T &;
	using const_reference = const T &;
	using size_type       = Size;
	using difference_type = std::ptrdiff_t;

#if OEL_MEM_BOUND_DEBUG_LVL
	using iterator       = debug::array_iterator< T *, _detail::FcaProxy<T, Size> >;
	using const_iterator = debug::array_iterator< const T *, _detail::FcaProxy<T, Size> >;
#else
	using iterator       = T *;
	using const_iterator = const T *;
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	constexpr fixcap_array() noexcept   : _size() {}

	/** @brief Elements are default initialized, meaning non-class T produces indeterminate values
	* @throw length_error if size > Capacity  */
	fixcap_array(size_type size, default_init_t);
	//! Throws length_error if size > Capacity. (Elements are value-initialized, same as std::vector)
	explicit fixcap_array(size_type size);
	fixcap_array(size_type size, const T & fillVal); //!< Throws length_error if size > Capacity

	template< typename InputRange,
	          typename /*EnableIfRange*/ = iterator_t<InputRange> >
	explicit fixcap_array(const InputRange & range)   : _size() { append(range); }

	fixcap_array(std::initializer_list<T> init)   : _size() { append<>(init); }

	fixcap_array(fixcap_array && other)
		noexcept(std::is_nothrow_move_constructible<T>::value or is_trivially_relocatable<T>::value);
	fixcap_array(const fixcap_array & other);

	~fixcap_array() noexcept   { _detail::Destroy(data(), data() + _size); }

	fixcap_array & operator =(fixcap_array && other) &;
	fixcap_array & operator =(const fixcap_array & other) &;

	fixcap_array & operator =(std::initializer_list<T> il) &  { assign(il);  return *this; }

	/**
	* @brief Replace the contents with source
	* @throw length_error if count > Capacity  */
	template<typename InputRange>
	auto assign(const InputRange & source)
	->  iterator_t<InputRange const>              { return _assign(_getSize(source, 0), source); }
	void assign(size_type count, const T & val)   { clear(); append(count, val); }

	/**
	* @brief Add at end the elements from range (in order)
	* @param source object which begin and end can be called on (an array, STL container or iterator_range)
	* @return begin(source) incremented to end of source
	*
	* Any previous end iterator will point to the first element added.
	* Strong exception safety, aka. commit or rollback semantics  */
	template<typename InputRange>
	auto append(const InputRange & source)
	->  iterator_t<InputRange const>            { return _append(_getSize(source, 0), source); }
	//! Same as `std::vector::insert(end(), il)`
	void append(std::initializer_list<T> il)    { append<>(il); }
	//! Same as `std::vector::insert(end(), count, val)`
	void append(size_type count, const T & val);

	/**
	* @brief Added elements are default initialized, meaning non-class T produces indeterminate values
	* @throw length_error if n > Capacity  */
	void      resize_default_init(size_type n)   { _resizeImpl(n, _detail::UninitDefaultConstructA<T>); }
	//! Throws length_error if n > Capacity. (Value-initializes added elements, same as std::vector::resize)
	void      resize(size_type n)                { _resizeImpl(n, _detail::UninitFillA{}); }

	template<typename ForwardRange>
	iterator  insert_r(const_iterator pos, const ForwardRange & source) &;

	iterator  insert(const_iterator pos, std::initializer_list<T> il) &  { return insert_r(pos, il); }

	iterator  insert(const_iterator pos, T && val) &       { return emplace(pos, std::move(val)); }
	iterator  insert(const_iterator pos, const T & val) &  { return emplace(pos, val); }

	template<typename... Args>
	iterator  emplace(const_iterator pos, Args &&... elemInitArgs) &;  //!< Throws length_error when full

	template<typename... Args>
	reference emplace_back(Args &&... args) &;  //!< Throws length_error when full

	//! Throws length_error when full
	void      push_back(T && val)       { emplace_back(std::move(val)); }
	//! Throws length_error when full
	void      push_back(const T & val)  { emplace_back(val); }

	void      pop_back() noexcept(nodebug);

	iterator  erase_unstable(iterator pos) &   { _eraseUnorder(pos, is_trivially_relocatable<T>());  return pos; }

	iterator  erase(iterator pos) &            { _erase(pos, is_trivially_relocatable<T>());  return pos; }

	iterator  erase(iterator first, const_iterator last) &;
	//! Equivalent to erase(first, end()) (but potentially faster), making first the new end
	void      erase_to_end(iterator first) noexcept(nodebug);

	void      clear() noexcept         { erase_to_end(begin()); }

	bool      empty() const noexcept   { return 0 == _size; }

	bool      full() const noexcept    { return Capacity == _size; }

	size_type size() const noexcept    OEL_ALWAYS_INLINE { return _size; }

	static constexpr size_type max_size() noexcept  { return Capacity; }

	iterator       begin() noexcept          OEL_ALWAYS_INLINE { return _makeIterator(_data); }
	const_iterator begin() const noexcept    OEL_ALWAYS_INLINE { return _makeConstIter(_data); }
	const_iterator cbegin() const noexcept   OEL_ALWAYS_INLINE { return _makeConstIter(_data); }

	iterator       end() noexcept          OEL_ALWAYS_INLINE { return _makeIterator(_data + _size); }
	const_iterator end() const noexcept    OEL_ALWAYS_INLINE { return _makeConstIter(_data + _size); }
	const_iterator cend() const noexcept   OEL_ALWAYS_INLINE { return end(); }

	reverse_iterator       rbegin() noexcept        OEL_ALWAYS_INLINE { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept  OEL_ALWAYS_INLINE { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() noexcept        OEL_ALWAYS_INLINE { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const noexcept  OEL_ALWAYS_INLINE { return const_reverse_iterator{begin()}; }

	T *       data() noexcept         { return reinterpret_cast<T *>(_data); }
	const T * data() const noexcept   { return reinterpret_cast<const T *>(_data); }

	reference       front() noexcept(nodebug)        { return operator[](0); }
	const_reference front() const noexcept(nodebug)  { return operator[](0); }

	reference       back() noexcept(nodebug)         { return operator[](_size - 1); }
	const_reference back() const noexcept(nodebug)   { return operator[](_size - 1); }

	reference       operator[](size_type index) noexcept(nodebug);
	const_reference operator[](size_type index) const noexcept(nodebug);

	template<size_t C1>
	friend bool operator==(const fixcap_array & left, const fixcap_array<T, C1, Size> & right)
		{
			return left.size() == right.size() and
			       std::equal(left.begin(), left.end(), right.begin());
		}
	template<size_t C1>
	friend bool operator!=(const fixcap_array & left, const fixcap_array<T, C1, Size> & right)
		{	return !(left == right); }
	template<size_t C1>
	friend bool operator <(const fixcap_array & left, const fixcap_array<T, C1, Size> & right)
		{
			return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
		}
	template<size_t C1>
	friend bool operator >(const fixcap_array & left, const fixcap_array<T, C1, Size> & right)  { return right < left; }



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
	size_type _size;
	aligned_union_t<T> _data[Capacity];


	iterator _makeIterator(aligned_union_t<T> * pos)
	{
		return _detail::ArrayIteratorMaker<iterator>
		{	reinterpret_cast<T *>(pos),
			reinterpret_cast<const _detail::FcaProxy<T, Size> *>(this)
		};
	}
	const_iterator _makeConstIter(const aligned_union_t<T> * pos) const
	{
		return _detail::ArrayIteratorMaker<const_iterator>
		{	reinterpret_cast<const T *>(pos),
			reinterpret_cast<const _detail::FcaProxy<T, Size> *>(this)
		};
	}

	[[noreturn]] static void _throwLackCap()
	{
		_detail::Throw::LengthError("Not enough space in fixcap_array");
	}


	template<typename Range> // pass dummy int to prefer this overload
	static auto _getSize(const Range & r, int) -> decltype( static_cast<size_t>(oel::ssize(r)) ) { return oel::ssize(r); }

	template<typename Range>
	static false_type _getSize(const Range &, long) { return {}; }

	template<typename Range>
	static auto _count(const Range & r, int) -> decltype( static_cast<size_t>(oel::ssize(r)) ) { return oel::ssize(r); }

	template<typename Range>
	static size_t _count(const Range & r, long)
	{
		auto first = oel::adl_begin(r);
		static_assert(std::is_base_of< std::forward_iterator_tag, iter_category<decltype(first)> >::value,
		              "insert_r requires that begin(source) is a ForwardIterator (multi-pass)");
		return std::distance(first, oel::adl_end(r));
	}


	size_type _unusedCapacity() const
	{
		return Capacity - _size;
	}

	void _setEmptyIf(true_type) { _size = 0; }

	OEL_ALWAYS_INLINE void _setEmptyIf(false_type) {}

#ifdef __GNUC__
	#pragma GCC diagnostic push
	#if __GNUC__ >= 8
	#pragma GCC diagnostic ignored "-Wclass-memaccess"
	#endif
#endif

	void _eraseUnorder(iterator pos, false_type) // non-trivial relocation
	{
		*pos = std::move(back());
		pop_back();
	}

	void _eraseUnorder(iterator const pos, true_type)
	{
		T *const ptr = std::addressof(*pos);
		ptr-> ~T();
		--_size;
		auto mem = reinterpret_cast<aligned_union_t<T> *>(ptr);
		*mem = _data[_size];  // relocate last element to pos
	}

	void _erase(iterator pos, true_type /*trivialRelocate*/)
	{
		T *const ptr = to_pointer_contiguous(pos);
		OEL_ASSERT(data() <= ptr and ptr < data() + _size);

		ptr-> ~T();
		T *const next = ptr + 1;
		size_t const nAfter = _size - (next - data());
		std::memmove(ptr, next, sizeof(T) * nAfter); // move [pos + 1, _end) to [pos, _end - 1)
		--_size;
	}

	void _erase(iterator pos, false_type)
	{
		iterator last = std::move(pos + 1, end(), pos);
		(*last).~T();
		--_size;
	}


	template<typename UninitFillFunc>
	void _resizeImpl(size_type newSize, UninitFillFunc initAdded)
	{
		if (Capacity < newSize)
			_throwLackCap();

		T *const oldEnd = data() + _size;
		T *const newEnd = data() + newSize;
		if (oldEnd < newEnd) // then construct new
			initAdded(oldEnd, newEnd);
		else // destroy old
			_detail::Destroy(newEnd, oldEnd);

		_size = newSize;
	}

	template<typename ContiguousIter>
	ContiguousIter _assignImpl(ContiguousIter first, size_type const count, true_type)
	{	// fastest assign
		_size = count;
		// Not portable for self assignment. Use memmove?
		_detail::MemcpyCheck(first, count, _data);

		return first + count;
	}

	template<typename InputIter>
	InputIter _assignImpl(InputIter src, size_type const count, false_type)
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
		{	// first assign to old elements
			src = copy(src, data(), data() + _size);
			while (_size < count)
			{	// put rest of new in uninitialized part
				_detail::ConstructA(data() + _size, *src);
				++src; ++_size;
			}
		}
		else
		{	// enough elements, copy new and destroy old
			T *const newEnd = data() + count;
			src = copy(src, data(), newEnd);
			_detail::Destroy(newEnd, data() + _size);
			_size = count;
		}
		return src;
	}

	template<typename SizedRange>
	auto _assign(size_t count, const SizedRange & src) -> iterator_t<SizedRange const>
	{
		if (Capacity >= count)
		{
			auto first = oel::adl_begin(src);
			return _assignImpl(first, count, can_memmove_with<T *, decltype(first)>());
		}
		_throwLackCap();
	}

	template<typename InputRange>
	auto _assign(false_type, const InputRange & src) -> iterator_t<InputRange const>
	{	// no fast way of getting size
		clear();
		return append<>(src);
	}

	template<typename ContiguousIter>
	ContiguousIter _appendImpl(ContiguousIter first, size_type const count, std::true_type)
	{	// use memcpy
		_detail::MemcpyCheck(first, count, data() + _size);
		_size += count;

		return first + count;
	}

	template<typename InputIter>
	InputIter _appendImpl(InputIter src, size_type const count, std::false_type)
	{	// slower copy
		size_type newSize = _size + count;
		struct {} a;
		src = _detail::UninitCopy(src, data() + _size, data() + newSize, a);
		_size = newSize;
		return src;
	}

	template<typename SizedRange>
	auto _append(size_t count, const SizedRange & src) -> iterator_t<SizedRange const>
	{
		if (_unusedCapacity() >= count)
		{
			auto first = oel::adl_begin(src);
			return _appendImpl(first, count, can_memmove_with<T *, decltype(first)>());
		}
		_throwLackCap();
	}

	template<typename InputRange>
	auto _append(false_type, const InputRange & src) -> iterator_t<InputRange const>
	{	// number of items unknown (slowest)
		auto f = oel::adl_begin(src); auto l = oel::adl_end(src);
		for (; f != l; ++f)
			emplace_back(*f);

		return f;
	}
};

template<typename T, size_t Capacity, typename Size>
fixcap_array<T, Capacity, Size>::fixcap_array(size_type size, default_init_t)
{
	if (Capacity < size)
		_throwLackCap();

	_size = size;
	_detail::UninitDefaultConstructA(data(), data() + size);
}

template<typename T, size_t Capacity, typename Size>
fixcap_array<T, Capacity, Size>::fixcap_array(size_type size)
{
	if (Capacity < size)
		_throwLackCap();

	_size = size;
	_detail::UninitFillA{}(data(), data() + size);
}

template<typename T, size_t Capacity, typename Size>
fixcap_array<T, Capacity, Size>::fixcap_array(size_type size, const T & val)
{
	if (Capacity < size)
		_throwLackCap();

	_size = size;
	_detail::UninitFillA{}(data(), data() + size, val);
}

template<typename T, size_t Capacity, typename Size>
inline fixcap_array<T, Capacity, Size>::fixcap_array(const fixcap_array & other)
 :	_size(other._size)
{
	struct {} a;
	_detail::UninitCopy(other.data(), data(), data() + _size, a);
}

template<typename T, size_t Capacity, typename Size>
inline fixcap_array<T, Capacity, Size>::fixcap_array(fixcap_array && other)
	noexcept(std::is_nothrow_move_constructible<T>::value or is_trivially_relocatable<T>::value)
 :	_size()
{
	_appendImpl(std::make_move_iterator(other.data()), other._size, is_trivially_relocatable<T>());
	other._setEmptyIf(is_trivially_relocatable<T>());
}

template<typename T, size_t Capacity, typename Size>
fixcap_array<T, Capacity, Size> &  fixcap_array<T, Capacity, Size>::
	operator =(fixcap_array && other) &
{
	_assignImpl(std::make_move_iterator(other.data()), other._size, is_trivially_relocatable<T>());
	other._setEmptyIf(is_trivially_relocatable<T>());
	return *this;
}

template<typename T, size_t Capacity, typename Size>
inline fixcap_array<T, Capacity, Size> &  fixcap_array<T, Capacity, Size>::
	operator =(const fixcap_array & other) &
{	// Bypassing Capacity check in _assign
	_assignImpl(other.data(), other._size, is_trivially_copyable<T>());
	return *this;
}

template<typename T, size_t Capacity, typename Size>
void fixcap_array<T, Capacity, Size>::append(size_type n, const T & val)
{
	if (_unusedCapacity() < n)
		_throwLackCap();

	size_type newSize = _size + n;
	_detail::UninitFillA{}(data() + _size, data() + newSize, val);
	_size = newSize;
}

template<typename T, size_t Capacity, typename Size> template<typename... Args>
T & fixcap_array<T, Capacity, Size>::emplace_back(Args &&... args) &
{
	if (Capacity > _size)
	{
		T *const pos = data() + _size;
		_detail::ConstructA(pos, static_cast<Args &&>(args)...);
		++_size;
		return *pos;
	}
	_throwLackCap();
}

template<typename T, size_t Capacity, typename Size> template<typename... Args>
typename fixcap_array<T, Capacity, Size>::iterator  fixcap_array<T, Capacity, Size>::
	emplace(const_iterator pos, Args &&... args) &
{
	(void) _detail::AssertTrivialRelocate<T>{};
	OEL_ASSERT(begin() <= pos and pos <= end());

	if (Capacity > _size)
	{
		auto const pPos = const_cast<T *>(to_pointer_contiguous(pos));
		size_t const nAfterPos = _size - (pPos - data());
		// Temporary in case constructor throws or source is an element of this array at pos or after
		aligned_union_t<T> tmp;
		_detail::ConstructA(reinterpret_cast<T *>(&tmp), static_cast<Args &&>(args)...);
		// Relocate [pos, end) to [pos + 1, end + 1), leaving memory at pos uninitialized (conceptually)
		std::memmove(pPos + 1, pPos, sizeof(T) * nAfterPos);
		++_size;

		std::memcpy(pPos, &tmp, sizeof(T)); // relocate the new element to pos

		return _makeIterator(reinterpret_cast<aligned_union_t<T> *>(pPos));
	}
	_throwLackCap();
}

template<typename T, size_t Capacity, typename Size> template<typename ForwardRange>
typename fixcap_array<T, Capacity, Size>::iterator  fixcap_array<T, Capacity, Size>::
	insert_r(const_iterator pos, const ForwardRange & src) &
{
	(void) _detail::AssertTrivialRelocate<T>{};
	OEL_ASSERT(begin() <= pos and pos <= end());

	auto first = oel::adl_begin(src);
	using CanMemmove = can_memmove_with<T *, decltype(first)>;

	size_t const n = _count(src, int{});
	if (_unusedCapacity() >= n)
	{
		auto const pPos = const_cast<T *>(to_pointer_contiguous(pos));
		size_t const bytesAfterPos = sizeof(T) * ( _size - (pPos - data()) );
		T *const dLast = pPos + n;
		// Relocate elements to make space, leaving [pos, pos + n) uninitialized (conceptually)
		std::memmove(dLast, pPos, bytesAfterPos);

		_size += n;
		// Construct new
		OEL_CONST_COND if (CanMemmove::value)
		{
			struct {} a;
			_detail::UninitCopy(first, pPos, dLast, a);
		}
		else
		{
			T * dest = pPos;
			OEL_TRY_
			{
				while (dest != dLast)
				{
					_detail::ConstructA(dest, *first);
					++first; ++dest;
				}
			}
			OEL_CATCH_ALL
			{	// relocate back to fill hole
				std::memmove(dest, dLast, bytesAfterPos);
				_size -= (dLast - dest);
				OEL_WHEN_EXCEPTIONS_ON(throw);
			}
		}

		return _makeIterator(reinterpret_cast<aligned_union_t<T> *>(pPos));
	}
	_throwLackCap();
}


template<typename T, size_t Capacity, typename Size>
inline void fixcap_array<T, Capacity, Size>::pop_back() noexcept(nodebug)
{
	OEL_ASSERT(0 < _size);
	--_size;
	data()[_size].~T();
}

template<typename T, size_t Capacity, typename Size>
typename fixcap_array<T, Capacity, Size>::iterator  fixcap_array<T, Capacity, Size>::
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

template<typename T, size_t Capacity, typename Size>
void fixcap_array<T, Capacity, Size>::erase_to_end(iterator newEnd) noexcept(nodebug)
{
	T *const first = to_pointer_contiguous(newEnd);
	OEL_ASSERT(data() <= first and first <= data() + _size);
	_detail::Destroy(first, data() + _size);
	_size = first - data();
}


template<typename T, size_t Capacity, typename Size>
inline T & fixcap_array<T, Capacity, Size>::operator[](size_type index) noexcept(nodebug)
{
	OEL_ASSERT(static_cast<size_t>(index) < static_cast<size_t>(_size));
	return data()[index];
}
template<typename T, size_t Capacity, typename Size>
inline const T & fixcap_array<T, Capacity, Size>::operator[](size_type index) const noexcept(nodebug)
{
	OEL_ASSERT(static_cast<size_t>(index) < static_cast<size_t>(_size));
	return data()[index];
}

namespace _detail
{
	template<typename T, typename Size>
	struct FcaProxy
	{
		Size size;
		T    data[1];

		bool DerefValid(const T * pos) const
		{
			return static_cast<size_t>(pos - data) < static_cast<size_t>(size);
		}
	};
}

} // namespace oel
