#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/array_iterator.h"
#include "auxi/inplace_growarr_detail.h"

#include <algorithm>

/** @file
*/

namespace oel
{

//! inplace_growarr is trivially relocatable if T is
template< typename T, size_t C, typename S >
is_trivially_relocatable<T> specify_trivial_relocate(inplace_growarr<T, C, S>);

//! Overloads generic unordered_erase(RandomAccessContainer &, Integral) (in range_algo.h)
template< typename T, size_t C, typename S >  inline
void unordered_erase(inplace_growarr<T, C, S> & a, ptrdiff_t index)  { a.unordered_erase(a.begin() + index); }

/**
* @brief Resizable array, statically allocated. Specify maximum size as template argument.
*
* Behaviour which equals that of std::vector is mostly not documented. */
template< typename T, size_t Capacity, typename Size/* = size_t*/ >
class inplace_growarr   : private _detail::InplaceGrowarrSpecial<T, Capacity, Size>
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
	* @throw length_error if size > Capacity  */
	inplace_growarr(size_type size, for_overwrite_t);
	//! Throws length_error if size > Capacity. (Elements are value-initialized, same as std::vector)
	explicit inplace_growarr(size_type size);
	//! Throws length_error if size > Capacity
	inplace_growarr(size_type size, const T & fillVal)  { append(size, fillVal); }

	template< typename InputRange,
	          typename /*EnableIfRange*/ = iterator_t<InputRange> >
	explicit inplace_growarr(InputRange && source)      { append(source); }

	inplace_growarr(std::initializer_list<T> init)      { append<>(init); }

	inplace_growarr & operator =(std::initializer_list<T> il) &  { assign(il);  return *this; }

	// TODO: try_assign, try_append, try_insert_range, and unchecked_push_back, unchecked_emplace_back

	/**
	* @brief Replace the contents with source
	* @throw length_error if count > Capacity  */
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
	->  iterator_t<InputRange>                { return _append(_getSize(source, 0), source); }
	//! Same as `std::vector::insert(end(), il)`
	void append(std::initializer_list<T> il)  { append<>(il); }
	//! Same as `std::vector::insert(end(), count, val)`
	void append(size_type count, const T & val);

	/**
	* @brief Added elements are default initialized, meaning non-class T produces indeterminate values
	* @throw length_error if n > Capacity  */
	void     resize_for_overwrite(size_type n)   { _resizeImpl(n, _detail::UninitDefaultConstructA<T>); }
	//! Throws length_error if n > Capacity. (Value-initializes added elements, same as std::vector::resize)
	void     resize(size_type n)                 { _resizeImpl(n, _detail::UninitFillA{}); }

	template< typename ForwardRange >
	iterator insert_range(const_iterator pos, ForwardRange && source) &;

	iterator insert(const_iterator pos, T && val) &       { return emplace(pos, std::move(val)); }
	iterator insert(const_iterator pos, const T & val) &  { return emplace(pos, val); }

	template< typename... Args >
	iterator emplace(const_iterator pos, Args &&... elemInitArgs) &;  //!< Throws length_error when full

	template< typename... Args >
	T &      emplace_back(Args &&... args) &;  //!< Throws length_error when full

	//! Throws length_error when full
	void     push_back(T && val)       { emplace_back(std::move(val)); }
	//! Throws length_error when full
	void     push_back(const T & val)  { emplace_back(val); }

	void     pop_back() noexcept(nodebug);

	iterator unordered_erase(iterator pos) &   { _eraseUnorder(pos, is_trivially_relocatable<T>());  return pos; }

	iterator erase(iterator pos) &             { _erase(pos, is_trivially_relocatable<T>());  return pos; }

	iterator erase(iterator first, const_iterator last) &;
	//! Equivalent to erase(first, end()) (but potentially faster), making first the new end
	void     erase_to_end(iterator first) noexcept(nodebug);

	void     clear() noexcept         { erase_to_end(begin()); }

	bool     empty() const noexcept   { return 0 == _size; }

	bool     full() const noexcept    { return Capacity == _size; }

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

	// T *       data() noexcept
	// const T * data() const noexcept
	using _base::data;

	T &       front() noexcept(nodebug)        { return operator[](0); }
	const T & front() const noexcept(nodebug)  { return operator[](0); }

	T &       back() noexcept(nodebug)         { return operator[](_size - 1); }
	const T & back() const noexcept(nodebug)   { return operator[](_size - 1); }

	T &       operator[](size_type index) noexcept(nodebug)
		{
			OEL_ASSERT(static_cast<size_t>(index) < static_cast<size_t>(_size));
			return data()[index];
		}
	const T & operator[](size_type index) const noexcept(nodebug)
		{
			OEL_ASSERT(static_cast<size_t>(index) < static_cast<size_t>(_size));
			return data()[index];
		}

	template< size_t C1 >
	friend bool operator==(const inplace_growarr & left, const inplace_growarr<T, C1, Size> & right)
		{
			return left.size() == right.size() and
			       std::equal(left.begin(), left.end(), right.begin());
		}
	template< size_t C1 >
	friend bool operator!=(const inplace_growarr & left, const inplace_growarr<T, C1, Size> & right)
		{
			return !(left == right);
		}
	friend bool operator <(const inplace_growarr & left, const inplace_growarr & right)
		{
			return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
		}
	friend bool operator >(const inplace_growarr & left, const inplace_growarr & right)  { return right < left; }



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
	using _base::_size;

	iterator _makeIterator(aligned_union_t<T> * pos)
	{
		return _detail::ArrayIteratorMaker<iterator>
		{	reinterpret_cast<T *>(pos),
			reinterpret_cast<const _detail::InplaceGrowarrProxy<T, Size> *>(this)
		};
	}
	const_iterator _makeConstIter(const aligned_union_t<T> * pos) const
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

	template< typename Range >
	static auto _count(Range & r, int) -> decltype((size_t) oel::ssize(r)) { return oel::ssize(r); }

	template< typename Range >
	static size_t _count(Range & r, long)
	{
		auto first = oel::adl_begin(r);
		static_assert(std::is_base_of< std::forward_iterator_tag, iter_category<decltype(first)> >::value,
		              "insert_range requires that begin(source) is a ForwardIterator (multi-pass)");
		return std::distance(first, oel::adl_end(r));
	}


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


	template< typename UninitFillFunc >
	void _resizeImpl(size_type newSize, UninitFillFunc initAdded)
	{
		if (Capacity < newSize)
			_detail::BadAlloc::raise();

		T *const oldEnd = data() + _size;
		T *const newEnd = data() + newSize;
		if (oldEnd < newEnd)
			initAdded(oldEnd, newEnd);
		else
			_detail::Destroy(newEnd, oldEnd);

		_size = newSize;
	}

	template< typename SizedRange >
	iterator_t<SizedRange> _assign(size_t count, SizedRange & src)
	{
		if (Capacity >= count)
		{
			auto first = oel::adl_begin(src);
			return this->doAssign(first, count, can_memmove_with<T *, decltype(first)>());
		}
		_detail::BadAlloc::raise();
	}

	template< typename InputRange >
	iterator_t<InputRange> _assign(false_type, InputRange & src)
	{	// no fast way of getting size
		clear();
		return append<>(src);
	}

	template< typename ContiguousIter >
	ContiguousIter _appendImpl(ContiguousIter first, size_type const count, std::true_type)
	{	// use memcpy
		_detail::MemcpyCheck(first, count, data() + _size);
		_size += count;

		return first + count;
	}

	template< typename InputIter >
	InputIter _appendImpl(InputIter src, size_type const count, std::false_type)
	{	// slower copy
		size_type newSize = _size + count;
		struct {} a;
		src = _detail::UninitCopy(src, data() + _size, data() + newSize, a);
		_size = newSize;
		return src;
	}

	template< typename SizedRange >
	iterator_t<SizedRange> _append(size_t count, SizedRange & src)
	{
		if (_unusedCapacity() >= count)
		{
			auto first = oel::adl_begin(src);
			return _appendImpl(first, count, can_memmove_with<T *, decltype(first)>());
		}
		_detail::BadAlloc::raise();
	}

	template< typename InputRange >
	iterator_t<InputRange> _append(false_type, InputRange & src)
	{	// number of items unknown (slowest)
		auto f = oel::adl_begin(src); auto l = oel::adl_end(src);
		for (; f != l; ++f)
			emplace_back(*f);

		return f;
	}
};

template< typename T, size_t Capacity, typename Size >
inplace_growarr<T, Capacity, Size>::inplace_growarr(size_type size, for_overwrite_t)
{
	if (Capacity < size)
		_detail::BadAlloc::raise();

	_detail::UninitDefaultConstructA(data(), data() + size);
	_size = size;
}

template< typename T, size_t Capacity, typename Size >
inplace_growarr<T, Capacity, Size>::inplace_growarr(size_type size)
{
	if (Capacity < size)
		_detail::BadAlloc::raise();

	_detail::UninitFillA{}(data(), data() + size);
	_size = size;
}


template< typename T, size_t Capacity, typename Size >
void inplace_growarr<T, Capacity, Size>::append(size_type n, const T & val)
{
	if (_unusedCapacity() < n)
		_detail::BadAlloc::raise();

	size_type newSize = _size + n;
	_detail::UninitFillA{}(data() + _size, data() + newSize, val);
	_size = newSize;
}

template< typename T, size_t Capacity, typename Size> template<typename... Args >
T & inplace_growarr<T, Capacity, Size>::emplace_back(Args &&... args) &
{
	if (Capacity > _size)
	{
		T *const pos = data() + _size;
		_detail::ConstructA(pos, static_cast<Args &&>(args)...);
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
		aligned_union_t<T> tmp;
		_detail::ConstructA(reinterpret_cast<T *>(&tmp), static_cast<Args &&>(args)...);
		// Relocate [pos, end) to [pos + 1, end + 1), leaving memory at pos uninitialized (conceptually)
		std::memmove(pPos + 1, pPos, sizeof(T) * nAfterPos);
		++_size;

		std::memcpy(pPos, &tmp, sizeof(T)); // relocate the new element to pos

		return _makeIterator(reinterpret_cast<aligned_union_t<T> *>(pPos));
	}
	_detail::BadAlloc::raise();
}

template< typename T, size_t Capacity, typename Size> template<typename ForwardRange >
typename inplace_growarr<T, Capacity, Size>::iterator  inplace_growarr<T, Capacity, Size>::
	insert_range(const_iterator pos, ForwardRange && src) &
{
	(void) _detail::AssertTrivialRelocate<T>{};
	OEL_ASSERT(begin() <= pos and pos <= end());

	auto first = oel::adl_begin(src);
	using CanMemmove = can_memmove_with<T *, decltype(first)>;

	size_t const n = _count(src, int{});
	if (_unusedCapacity() >= n)
	{
		auto const pPos = const_cast<T *>(to_pointer_contiguous(pos));
		size_t const bytesAfterPos = sizeof(T) * ((data() + _size) - pPos);
		T *const dLast = pPos + n;
		// Relocate elements to make space, leaving [pos, pos + n) uninitialized (conceptually)
		std::memmove(dLast, pPos, bytesAfterPos);
		_size += n;
		// Construct new
		OEL_CONST_COND if (CanMemmove::value)
		{
			_detail::UninitCopyA(first, n, pPos, CanMemmove());
		}
		else
		{	T * dest = pPos;
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
	_detail::BadAlloc::raise();
}


template< typename T, size_t Capacity, typename Size >
inline void inplace_growarr<T, Capacity, Size>::pop_back() noexcept(nodebug)
{
	OEL_ASSERT(0 < _size);
	--_size;
	data()[_size].~T();
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

template< typename T, size_t Capacity, typename Size >
void inplace_growarr<T, Capacity, Size>::erase_to_end(iterator newEnd) noexcept(nodebug)
{
	T *const first = to_pointer_contiguous(newEnd);
	OEL_ASSERT(data() <= first and first <= data() + _size);
	_detail::Destroy(first, data() + _size);
	_size = first - data();
}

} // namespace oel
