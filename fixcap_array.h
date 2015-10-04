#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "contiguous_iterator.h"
#include "container_shared.h"

#include <stdexcept>


namespace oel
{

template<typename, size_t, typename> class fixcap_array;  // forward declare


using std::out_of_range;
using std::length_error;


template<typename T, size_t C, typename S>
is_trivially_relocatable<T> specify_trivial_relocate(fixcap_array<T, C, S> &&);

/**
* @brief Erase the element at position from fixcap_array without maintaining order of elements.
*
* Constant complexity (compared to linear in the distance between position and last for normal erase).
* @return Iterator pointing to the location that followed the element erased,
* which is the end if position was at the last element. */
template<typename T, size_t C, typename S>
typename fixcap_array<T, C, S>::iterator  erase_unordered(fixcap_array<T, C, S> & ctr,
														  typename fixcap_array<T, C, S>::iterator position);

template<typename T, size_t C, typename S>
bool operator==(const fixcap_array<T, C, S> & left, const fixcap_array<T, C, S> & right)
{
	return left.size() == right.size() &&
		   std::equal(left.begin(), left.end(), right.begin());
}
template<typename T, size_t C, typename S> inline
bool operator!=(const fixcap_array<T, C, S> & left, const fixcap_array<T, C, S> & right)  { return !(left == right); }

/**
* @brief Resizable array, statically allocated. Specify maximum size as template argument.
*
* Behaviour which equals that of std::vector is mostly not documented. */
template<typename T, size_t Capacity, typename Size = size_t>
class fixcap_array
{
public:
	using value_type      = T;
	using pointer         = T *;
	using const_pointer   = const T *;
	using reference       = T &;
	using const_reference = const T &;
	using size_type       = Size;
	using difference_type = std::ptrdiff_t;

	using iterator       = contiguous_iterator< T *, fixcap_array<T, Capacity, Size> >;
	using const_iterator = contiguous_iterator< const T *, fixcap_array<T, Capacity, Size> >;

	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	fixcap_array() noexcept  : _size(0) {}
	/**
	* @brief Elements are default initialized, meaning non-class T produces indeterminate values
	* @throw length_error if size > Capacity  */
	fixcap_array(size_type size, default_init_tag);
	fixcap_array(size_type size, const T & val); ///< Throws length_error if size > Capacity
	fixcap_array(std::initializer_list<T> init);

	fixcap_array(const fixcap_array & other);
	fixcap_array(fixcap_array && other);

	~fixcap_array() NOEXCEPT  { _detail::Destroy(data(), data() + _size); }

	fixcap_array & operator =(const fixcap_array & other);
	fixcap_array & operator =(fixcap_array && other);

	fixcap_array & operator =(std::initializer_list<T> il)  { assign(il);  return *this; }

	/**
	* @brief Replace the contents with count copies from range beginning at first
	* @throw length_error if count > Capacity  */
	template<typename InputIterator>
	InputIterator assign(InputIterator first, size_type count);
	/// Throws length_error if number of elements exceeds Capacity
	template<typename InputRange>
	void          assign(const InputRange & source)  { _assign(_getSize(source, int{}), source); }

	/**
	* @brief Add count elements at end from range beginning at first (in order)
	* @param first iterator to first element to append
	* @param count number of elements to append
	* @return end of input range
	* @throw length_error if size() + count > Capacity
	*
	*  Any previous past-the-end iterator will point to the first element added.
	* Strong exception safety, aka. commit or rollback semantics  */
	template<typename InputIterator>
	InputIterator append(InputIterator first, size_type count);
	/**
	* @brief Add at end the elements from range (in order)
	* @param range object which begin and end can be called on (an array, STL container or iterator_range)
	* @return iterator pointing to first element added, or end if range is empty
	*
	* Otherwise same as append(InputIterator, size_type)  */
	template<typename InputRange>
	iterator      append(const InputRange & range);
<<<<<<< HEAD
	/// Equivalent to calling append(const InputRange &) with il as argument  */
	iterator      append(std::initializer_list<T> il)  { return append<>(il); }

	template<typename... Args>
	void          emplace_back(Args &&... args);  ///< Throws length_error when full

	/// Throws length_error when full
	void          push_back(T && val)       { emplace_back(std::move(val)); }
	/// Throws length_error when full
	void          push_back(const T & val)  { emplace_back(val); }

	template<typename... Args>
	iterator      emplace(const_iterator position, Args &&... args);  ///< Throws length_error when full

	iterator      insert(const_iterator position, T && val)       { return emplace(position, std::move(val)); }
	iterator      insert(const_iterator position, const T & val)  { return emplace(position, val); }

	void          pop_back() NOEXCEPT;

	iterator      erase(iterator position) NOEXCEPT;

	iterator      erase(iterator first, iterator last) NOEXCEPT;

	/// Equivalent to erase(first, end()) (but potentially faster)
	void          erase_back(iterator first) NOEXCEPT;

	/**
	* @brief Added elements are default initialized, meaning non-class T produces indeterminate values
	* @throw length_error if new_size > Capacity  */
	void          resize(size_type newSize, default_init_tag);
	void          resize(size_type newSize, const T & addVal);  ///< Throws length_error if new_size > Capacity

	void          clear() NOEXCEPT        { erase_back(begin()); }

	bool          empty() const noexcept  { return 0 == _size; }

	size_type     size() const NOEXCEPT   { return _size; }

	static size_type max_size() NOEXCEPT  { return Capacity; }

	iterator        begin() noexcept         { return OEL_CONTIGUOUS_ITERATOR(data()); }
	const_iterator  begin() const noexcept   { return OEL_CONTIGUOUS_CONST_ITER(data()); }
	const_iterator  cbegin() const noexcept  { return begin(); }

	iterator        end() noexcept           { return OEL_CONTIGUOUS_ITERATOR(data() + _size); }
	const_iterator  end() const noexcept     { return OEL_CONTIGUOUS_CONST_ITER(data() + _size); }
	const_iterator  cend() const noexcept    { return end(); }

	reverse_iterator       rbegin() NOEXCEPT        { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const NOEXCEPT  { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() NOEXCEPT          { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const NOEXCEPT    { return const_reverse_iterator{begin()}; }

	pointer         data() NOEXCEPT        { return reinterpret_cast<pointer>(_data); }
	const_pointer   data() const NOEXCEPT  { return reinterpret_cast<const_pointer>(_data); }

	reference       front() NOEXCEPT        { return operator[](0); }
	const_reference front() const NOEXCEPT  { return operator[](0); }

	reference       back() NOEXCEPT         { return operator[](_size - 1); }
	const_reference back() const NOEXCEPT   { return operator[](_size - 1); }

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) NOEXCEPT;
	const_reference operator[](size_type index) const NOEXCEPT;


////////////////////////////////////////////////////////////////////////////////

// Implementation only in rest of the file


private:
	size_type _size;
	aligned_storage_t<sizeof(T), OEL_ALIGNOF(T)> _data[Capacity];


	static length_error _lengthExc()
	{
		return length_error("Not enough space in fixcap_array");
	}

	template<typename Range>
	static auto _getSize(const Range & r, int) -> decltype(r.size()) { return r.size(); }

	template<typename Range>
	static auto _getSize(const Range & r, long) -> decltype(::adl_end(r) - ::adl_begin(r))
												   { return ::adl_end(r) - ::adl_begin(r); }

	static std::false_type _getSize(...) { return {}; }


	size_type _unusedCapacity() const NOEXCEPT
	{
		return Capacity - _size;
	}

	void _uninitCopyFrom(std::false_type, const fixcap_array<T, Capacity, Size> & src)
	{	// non-trivial copy
		_size = src._size;
		oel::uninitialized_copy_n(src.data(), src._size, data());
	}

	void _uninitCopyFrom(std::true_type, const fixcap_array<T, Capacity, Size> & src)
	{	// trivial copy
		//_size = src._size;
		//::memcpy(data(), src.data(), src._size * sizeof(T));
		::memcpy(this, &src, sizeof src._size + src._size * sizeof(T));
	}

	void _setEmptyIfNot(std::false_type)
	{
		_size = 0;
	}

	void _setEmptyIfNot(std::true_type) {}

	template<typename UninitFillFunc>
	void _resizeImpl(size_type newSize, UninitFillFunc initNewElems)
	{
		if (Capacity >= newSize)
		{
			pointer const oldEnd = data() + _size;
			pointer const newEnd = data() + newSize;
			if (oldEnd < newEnd) // then construct new
				initNewElems(oldEnd, newEnd);
			else // destroy old
				_detail::Destroy(newEnd, oldEnd);

			_size = newSize;
		}
		else
		{	throw _lengthExc();
		}
	}

	template<typename CntigusIter>
	CntigusIter _assignInternal(std::true_type, CntigusIter first, size_type const count)
	{	// fastest assign
		_size = count;
		// Not portable for self assignment. Use memmove?
		::memcpy(data(), to_pointer_contiguous(first), count * sizeof(T));

		return first + count;
	}

	template<typename InputIter>
	InputIter _assignInternal(std::false_type, InputIter src, size_type const count)
	{	// cannot use memcpy
		difference_type const diff = count - _size;
		if (0 < diff)
		{	// assign to old elements and construct rest
			auto const res = oel::copy_nonoverlap(src, _size, data());
			src = oel::uninitialized_copy_n(res.src_end, diff, res.dest_end).src_end;
		}
		else
		{	// enough elements, copy new and destroy old
			auto const res = oel::copy_nonoverlap(src, count, data());
			src = res.src_end;
			_detail::Destroy(res.dest_end, data() + _size);
		}
		_size = count;
		return src;
	}

	template<typename SizeRange>
	void _assign(size_type count, const SizeRange & range)
	{
		if (Capacity >= count)
		{
			auto first = ::adl_begin(range);
			_assignInternal(can_memmove_with<pointer, decltype(first)>(), first, count);
		}
		else
		{	throw _lengthExc();
		}
	}

	template<typename InputRange>
	void _assign(std::false_type, const InputRange & range)
	{	// no fast way of getting size
		clear();
		append(range);
	}

	template<typename CntigusIter>
	CntigusIter _appendN(std::true_type, CntigusIter first, size_type const count)
	{	// use memcpy
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (count > 0)
		{	// Dereference to catch out of range errors if the iterators have internal checks
			(void)*first;
			(void)*(first + (count - 1));
		}
	#endif
		::memcpy(data() + _size, to_pointer_contiguous(first), count * sizeof(T));
		_size += count;

		return first + count;
	}

	template<typename InputIter>
	InputIter _appendN(std::false_type, InputIter src, size_type const count)
	{	// slower copy
		src = oel::uninitialized_copy_n(src, count, data() + _size).src_end;
		_size += count;
		return src;
	}

	template<typename CntigusIter>
	void _appendInternal(std::true_type, CntigusIter first, CntigusIter, size_type count)
	{	// fast copy
		_appendN(std::true_type{}, first, count);
	}

	template<typename InputIter>
	void _appendInternal(std::false_type, InputIter first, InputIter last, size_type count)
	{	// cannot fast copy
		std::uninitialized_copy(first, last, data() + _size);
		_size += count;
	}

	template<typename SizeRange>
	void _append(size_type count, const SizeRange & range)
	{
		if (_unusedCapacity() >= count)
		{
			auto first = ::adl_begin(src);
			_appendInternal(can_memmove_with<pointer, decltype(first)>(),
							first, ::adl_end(src), count);
		}
		else
		{	throw _lengthExc();
		}
	}

	template<typename InputRange>
	void _append(std::false_type, const InputRange & range)
	{	// number of items unknown (slowest)
		for (auto && v : range)
			push_back( std::forward<decltype(v)>(v) );
	}
};

template<typename T, size_t Capacity, typename Size>
fixcap_array<T, Capacity, Size>::fixcap_array(size_type size, default_init_tag)
{
	if (Capacity >= size)
	{
		_size = size;
		_detail::UninitFillDefault(data(), data() + size);
	}
	else
	{	_throwLenExc();
	}
}

template<typename T, size_t Capacity, typename Size>
fixcap_array<T, Capacity, Size>::fixcap_array(size_type size)
{
	if (Capacity >= size)
	{
		_size = size;
		oel::uninitialized_fill_default(data(), data() + size);
	}
	else
	{	throw _lengthExc();
	}
}

template<typename T, size_t Capacity, typename Size>
fixcap_array<T, Capacity, Size>::fixcap_array(size_type size, const T & val)
{
	if (Capacity >= size)
	{
		_size = size;
		std::uninitialized_fill_n(data(), size, val);
	}
	else
	{	throw _lengthExc();
	}
}

template<typename T, size_t Capacity, typename Size>
inline fixcap_array<T, Capacity, Size>::fixcap_array(const fixcap_array & other)
{
	_uninitCopyFrom(is_trivially_copyable<T>(), other);
}

template<typename T, size_t Capacity, typename Size>
inline fixcap_array<T, Capacity, Size>::fixcap_array(fixcap_array && other)
{
	_detail::AssertRelocate<T>();

	_uninitCopyFrom(std::true_type{}, other);
	other._setEmptyIfNot(std::is_trivially_destructible<T>());
}

template<typename T, size_t Capacity, typename Size>
fixcap_array<T, Capacity, Size> &  fixcap_array<T, Capacity, Size>::
	operator =(const fixcap_array & other)
{	// Bypassing Capacity check in _assign
	_assignInternal(is_trivially_copyable<T>(), other.data(), other._size);
	return *this;
}

template<typename T, size_t Capacity, typename Size>
inline fixcap_array<T, Capacity, Size> &  fixcap_array<T, Capacity, Size>::
	operator =(fixcap_array && other)
{
	_detail::AssertRelocate<T>();

	_assignInternal(std::true_type{}, other.data(), other._size);
	other._setEmptyIfNot(std::has_trivial_destructor<T>());
	return *this;
}

template<typename T, size_t Capacity, typename Size> template<typename InputIterator>
InputIterator fixcap_array<T, Capacity, Size>::assign(InputIterator first, size_type count)
{
	if (Capacity >= count)
		return _assignInternal(can_memmove_with<pointer, InputIterator>, first, count);
	else
		throw _lengthExc();
}

template<typename T, size_t Capacity, typename Size> template<typename InputIterator>
InputIterator fixcap_array<T, Capacity, Size>::append(InputIterator first, size_type count)
{
	if (_unusedCapacity() >= count)
		return _appendN(can_memmove_with<pointer, InputIterator>, first, count);
	else
		throw _lengthExc();
}

template<typename T, size_t Capacity, typename Size> template<typename InputRange>
typename fixcap_array<T, Capacity, Size>::iterator  fixcap_array<T, Capacity, Size>::
	append(const InputRange & src)
{
	iterator const oldEnd = end();

	_append(_getSize(source, int{}), source);

	return oldEnd;
}

template<typename T, size_t Capacity, typename Size> template<typename... Args>
void fixcap_array<T, Capacity, Size>::emplace_back(Args &&... args)
{
	if (Capacity > _size)
	{
		::new(data() + _size) T(std::forward<Args>(args)...); // direct-initialize, or value-initialize if no args
		++_size;
	}
	else
	{	throw _lengthExc();
	}
}

template<typename T, size_t Capacity, typename Size> template<typename... Args>
typename fixcap_array<T, Capacity, Size>::iterator  fixcap_array<T, Capacity, Size>::
	emplace(const_iterator pos, Args &&... args)
{
	_detail::AssertRelocate<T>();

	MEM_BOUND_ASSERT_CHEAP(begin() <= pos && pos <= end());

	if (Capacity > _size)
	{
		auto const posPtr = const_cast<pointer>(to_pointer_contiguous(pos));
		size_type const nAfterPos = (data() + _size) - posPtr;

		// Temporary in case constructor throws or source is an element of this array at pos or after
		using RawStore = aligned_storage_t<sizeof(T), OEL_ALIGNOF(T)>;
		RawStore local;
		::new(&local) T(std::forward<ArgTs>(args)...);
		// Move [pos, end) to [pos + 1, end + 1), conceptually destroying element at pos
		::memmove(posPtr + 1, posPtr, nAfterPos * sizeof(T));
		++_end;

		*reinterpret_cast<RawStore *>(posPtr) = local; // relocate the new element to pos

		return OEL_CONTIGUOUS_ITERATOR(posPtr);
	}
	else
	{	throw _lengthExc();
	}
}

template<typename T, size_t Capacity, typename Size>
inline void fixcap_array<T, Capacity, Size>::resize(size_type newSize, default_init_tag)
{
	OEL_MEM_BOUND_ASSERT(0 < _size);
	--_size;
	data()[_size].~T();
}

template<typename T, size_t Capacity, typename Size>
inline void fixcap_array<T, Capacity, Size>::pop_back() noexcept
{
	_resizeImpl(newSize, oel::uninitialized_fill_default<pointer>);
}

template<typename T, size_t Capacity>
void fixcap_array<T, Capacity>::resize(size_type newSize, const T & addVal)
{
	_resizeImpl( newSize,
			[&addVal](pointer first, pointer last)
			{
				std::uninitialized_fill(first, last, addVal);
			} );
}

template<typename T, size_t Capacity, typename Size>
typename fixcap_array<T, Capacity, Size>::iterator  fixcap_array<T, Capacity, Size>::
	erase(iterator pos)
{
	_detail::AssertRelocate<T>();

	pointer const posPtr = to_pointer_contiguous(pos);
	MEM_BOUND_ASSERT_CHEAP(data() <= posPtr && pos < end());

	posPtr-> ~T();
	pointer const next = to_pointer_contiguous(pos) + 1;
	::memmove(to_pointer_contiguous(pos), next, ((data() + _size) - next) * sizeof(T)); // move [pos + 1, _end) to [pos, _end - 1)
	--_size;
	return pos;
}

template<typename T, size_t Capacity, typename Size>
typename fixcap_array<T, Capacity, Size>::iterator  fixcap_array<T, Capacity, Size>::
	erase(iterator first, iterator last)
{
	_detail::AssertRelocate<T>();

	pointer const pFirst = to_pointer_contiguous(first);
	pointer const pLast = to_pointer_contiguous(last);
	MEM_BOUND_ASSERT_CHEAP(data() <= pFirst);
	OEL_MEM_BOUND_ASSERT(pFirst <= pLast && last <= end()); // memmove will crash if last > end

	difference_type nErase = pLast - pFirst;
	if (0 < nErase)
	{
		_detail::Destroy(pFirst, pLast);
		size_type const nAfterLast = (data() + _size) - pLast;
		// move [last, _end) to [first, first + nAfterLast)
		::memmove(pFirst, pLast, nAfterLast * sizeof(T));
		_size -= nErase;
	}
	return first;
}

template<typename T, size_t Capacity, typename Size>
void fixcap_array<T, Capacity, Size>::erase_back(iterator newEnd) noexcept
{
	pointer const first = to_pointer_contiguous(newEnd);
	MEM_BOUND_ASSERT_CHEAP(data() <= first && first <= data() + _size);
	_detail::Destroy(first, data() + _size);
	_size = first - data();
}

template<typename T, size_t Capacity, typename Size>
inline typename fixcap_array<T, Capacity, Size>::reference  fixcap_array<T, Capacity, Size>::
	at(size_type index)
{
	if (index < _size)
		return data()[index];
	else
		throw out_of_range("Invalid index fixcap_array::at");
}
template<typename T, size_t Capacity, typename Size>
typename fixcap_array<T, Capacity, Size>::const_reference  fixcap_array<T, Capacity, Size>::
	at(size_type index) const
{
	if (index < _size)
		return data()[index];
	else
		throw out_of_range("Invalid index fixcap_array::at");
}

template<typename T, size_t Capacity, typename Size>
inline typename fixcap_array<T, Capacity, Size>::reference  fixcap_array<T, Capacity, Size>::
	operator[](size_type index) noexcept
{
	OEL_MEM_BOUND_ASSERT(index < _size);
	return data()[index];
}
template<typename T, size_t Capacity, typename Size>
inline typename fixcap_array<T, Capacity, Size>::const_reference  fixcap_array<T, Capacity, Size>::
	operator[](size_type index) const noexcept
{
	OEL_MEM_BOUND_ASSERT(index < _size);
	return data()[index];
}

#undef OEL_CONTIGUOUS_ITERATOR
#undef OEL_CONTIGUOUS_CONST_ITER

} // namespace oel

template<typename T, size_t C, typename S>
inline typename oel::fixcap_array<T, C>::iterator  oel::
	erase_unordered(fixcap_array<T, C> & ctr, typename fixcap_array<T, C>::iterator pos)
{
	*pos = std::move(ctr.back());
	ctr.pop_back();
	return pos;
}