#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "array_iterator.h"
#include "container_shared.h"


namespace oetl
{

template<typename, size_t> class fixcap_array;  // forward declare


using std::out_of_range;
using std::length_error;


template<typename T, size_t C>
struct is_trivially_relocatable< fixcap_array<T, C> > :
	is_trivially_relocatable<T> {};

/**
* @brief Erase the element at position from fixcap_array without maintaining order of elements.
*
* Constant complexity (compared to linear in the distance between position and last for normal erase).
* @return Iterator pointing to the location that followed the element erased,
* which is the end if position was at the last element. */
template<typename T, size_t C>
typename fixcap_array<T, C>::iterator  erase_unordered(fixcap_array<T, C> & ctr,
													   typename fixcap_array<T, C>::iterator position);

/// Non-member truncate, overloads generic one (stl_util.h)
template<typename T, size_t C> inline
void truncate(fixcap_array<T, C> & ctr, typename fixcap_array<T, C>::iterator newEnd)  { ctr.erase_from(newEnd); }

template<typename T, size_t Capacity>
bool operator==(const fixcap_array<T, Capacity> & left, const fixcap_array<T, Capacity> & right);
template<typename T, size_t Capacity> inline
bool operator!=(const fixcap_array<T, Capacity> & left, const fixcap_array<T, Capacity> & right)  { return !(left == right); }

/**
* @brief Resizable array, statically allocated. Specify maximum size as template argument.
*
* Behaviour which equals that of std::vector is mostly not documented. */
template<typename T, size_t Capacity>
class fixcap_array
{
public:
	using value_type      = T;
	using pointer         = T *;
	using const_pointer   = const T *;
	using reference       = T &;
	using const_reference = const T &;

#if OETL_MEM_BOUND_DEBUG_LVL
	using iterator       = array_iterator< fixcap_array<T, Capacity> >;
	using const_iterator = array_const_iterator< fixcap_array<T, Capacity> > ;
#else
	using iterator       = T *;
	using const_iterator = const T *;
#endif
	using difference_type = typename std::iterator_traits<iterator>::difference_type;
	using size_type       = size_t;

	fixcap_array() NOEXCEPT;
	/**
	* @brief Non-trivial constructor called on elements, otherwise not initialized.
	* @throw length_error if size > Capacity  */
	fixcap_array(size_type size);
	fixcap_array(size_type size, const T & val); ///< Throws length_error if size > Capacity

	fixcap_array(const fixcap_array & other);
	fixcap_array(fixcap_array && other);

	~fixcap_array() NOEXCEPT;

	fixcap_array & operator =(const fixcap_array & other);
	fixcap_array & operator =(fixcap_array && other);

	/**
	* @brief Replace the contents with count copies from range beginning at first
	* @throw length_error if count > Capacity  */
	template<typename InputIterator>
	InputIterator assign(InputIterator first, size_type count);
	/// Throws length_error if number of elements exceeds Capacity
	template<typename InputRange>
	void          assign(const InputRange & range);

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

	template<typename... Params>
	void          push_back(Params &&... args);  ///< Throws length_error when full

	template<typename... Params>
	iterator      insert(const_iterator position, Params &&... args);

	void          pop_back() NOEXCEPT;		///< Does assert(!empty()) internally

	iterator      erase(iterator position);

	iterator      erase(iterator first, iterator last);

	/// Pop the elements from newEnd to the end of fixcap_array
	void          erase_from(iterator newEnd) NOEXCEPT;

	/**
	* @brief Non-trivial constructor called on added elements, otherwise not initialized.
	* @throw length_error if new_size > Capacity  */
	void          resize(size_type newSize);
	void          resize(size_type newSize, const T & addVal);  ///< Throws length_error if new_size > Capacity

	void          clear() NOEXCEPT;

	bool          empty() const NOEXCEPT;

	size_type     size() const NOEXCEPT   { return _size; }

	static size_type max_size() NOEXCEPT  { return Capacity; }

	iterator        begin() NOEXCEPT;
	const_iterator  begin() const NOEXCEPT;

	iterator        end() NOEXCEPT;
	const_iterator  end() const NOEXCEPT;

	pointer         data() NOEXCEPT;
	const_pointer   data() const NOEXCEPT;

	reference       front() NOEXCEPT;
	const_reference front() const NOEXCEPT;

	reference       back() NOEXCEPT;
	const_reference back() const NOEXCEPT;

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) NOEXCEPT;
	const_reference operator[](size_type index) const NOEXCEPT;


////////////////////////////////////////////////////////////////////////////////

// Implementation only in rest of the file


private:
	size_type _size;
	_detail::AlignedStorage<sizeof(T), std::alignment_of<T>::value>
			  _data[Capacity];


	template<typename CheckT>
	struct _staticAssertTrivialRelocate
	{
		static_assert(is_trivially_relocatable<CheckT>::value,
			"Template argument T must be trivially relocatable, see documentation for is_trivially_relocatable");
	};

	static length_error _lengthExc()
	{
		return length_error("Not enough space in fixcap_array");
	}

	template<typename Range>
	static auto _getSize(const Range & r, int) -> decltype(r.size())
	{
		return r.size();
	}

	template<typename Range>
	static auto _getSize(const Range & r, long) -> decltype(adl_end(r) - adl_begin(r))
	{
		return adl_end(r) - adl_begin(r);
	}

	static std::false_type _getSize(...) { return {}; }


#if OETL_MEM_BOUND_DEBUG_LVL
#	define OEP_FIXCAPARR_ITERATOR(ptr)        iterator{ptr, this}
#	define OEP_FIXCAPARR_CONST_ITER(constPtr) const_iterator{constPtr, this}
#else
#	define OEP_FIXCAPARR_ITERATOR(ptr)        (ptr)
#	define OEP_FIXCAPARR_CONST_ITER(constPtr) (constPtr)
#endif

	size_type _unusedCapacity() const NOEXCEPT
	{
		return Capacity - _size;
	}

	void _uninitCopyFrom(std::false_type, const fixcap_array<T, Capacity> & src)
	{	// non-trivial copy
		_size = src._size;
		oetl::uninitialized_copy_n(src.data(), src._size, data());
	}

	void _uninitCopyFrom(std::true_type, const fixcap_array<T, Capacity> & src)
	{	// trivial copy
		//_size = src._size;
		//memcpy(data(), src.data(), src._size * sizeof(T));
		memcpy(this, &src, sizeof src._size + src._size * sizeof(T));
	}

	void _setEmptyIfNot(std::false_type)
	{
		_size = 0;
	}

	void _setEmptyIfNot(std::true_type) {
	}

	template<typename UninitFillFunc>
	void _resizeImpl(size_type newSize, UninitFillFunc initNewElems)
	{
		pointer const oldEnd = data() + _size;
		pointer const newEnd = data() + newSize;
		if (oldEnd < newEnd) // then construct new
			initNewElems(oldEnd, newEnd);
		else // destroy old
			_detail::Destroy(newEnd, oldEnd);

		_size = newSize;
	}

	template<typename CntigusIter>
	CntigusIter _assignInternal(std::true_type, CntigusIter first, size_type const count)
	{	// fastest assign
		_size = count;
		// Not portable for self assignment. Use memmove?
		memcpy(data(), to_ptr(first), count * sizeof(T));

		return first + count;
	}

	template<typename InputIter>
	InputIter _assignInternal(std::false_type, InputIter src, size_type const count)
	{	// cannot use memcpy
		difference_type const diff = count - _size;
		if (0 < diff)
		{	// assign to old elements and construct rest
			auto const res = oetl::copy_nonoverlap(src, _size, data());
			src = oetl::uninitialized_copy_n(res.src_end, diff, res.dest_end).src_end;
		}
		else
		{	// enough elements, copy new and destroy old
			auto const res = oetl::copy_nonoverlap(src, count, data());
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
			auto first = adl_begin(range);
			_assignInternal(can_memmove_ranges_with(data(), first), first, count);
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
#	if OETL_MEM_BOUND_DEBUG_LVL
		if (count > 0)				// Dereference iterator to the last element to append,
			*(first + (count - 1)); // this catches out of range errors with checked iterators
#	endif
		memcpy(data() + _size, to_ptr(first), count * sizeof(T));
		_size += count;

		return first + count;
	}

	template<typename InputIter>
	InputIter _appendN(std::false_type, InputIter src, size_type const count)
	{	// slower copy
		src = oetl::uninitialized_copy_n(src, count, data() + _size).src_end;
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
			auto first = adl_begin(range);
			_appendInternal(can_memmove_ranges_with(data(), first),
							first, adl_end(range), count);
		}
		else
		{	throw _lengthExc();
		}
	}

	template<typename InputRange>
	void _append(std::false_type, const InputRange & range)
	{	// number of items unknown (slowest)
		iterator const oldEnd = end();
		try
		{
			for (auto && v : range)
				push_back( std::forward<decltype(v)>(v) );
		}
		catch (...)
		{
			erase_from(oldEnd);
			throw;
		}
	}
};

// Definitions of public functions

template<typename T, size_t Capacity>
inline fixcap_array<T, Capacity>::
fixcap_array() NOEXCEPT :
	_size(0) {
}

template<typename T, size_t Capacity>
fixcap_array<T, Capacity>::
fixcap_array(size_type size)
{
	if (Capacity >= size)
	{
		_size = size;
		oetl::uninitialized_fill_default(data(), data() + size);
	}
	else
	{	throw _lengthExc();
	}
}

template<typename T, size_t Capacity>
fixcap_array<T, Capacity>::
fixcap_array(size_type size, const T & val)
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

template<typename T, size_t Capacity>
inline fixcap_array<T, Capacity>::
~fixcap_array() NOEXCEPT
{
	_detail::Destroy(data(), data() + _size);
}

template<typename T, size_t Capacity>
inline fixcap_array<T, Capacity>::
fixcap_array(const fixcap_array<T, Capacity> & other)
{
	_uninitCopyFrom(is_trivially_copyable<T>(), other);
}

template<typename T, size_t Capacity>
inline fixcap_array<T, Capacity>::
fixcap_array(fixcap_array<T, Capacity> && other)
{
	_staticAssertTrivialRelocate<T>();

	_uninitCopyFrom(std::true_type{}, other);
	other._setEmptyIfNot(std::has_trivial_destructor<T>());
}

template<typename T, size_t Capacity>
inline fixcap_array<T, Capacity> &  fixcap_array<T, Capacity>::
	operator =(const fixcap_array<T, Capacity> & other)
{	// Bypassing Capacity check in _assign
	_assignInternal(is_trivially_copyable<T>(), other.data(), other._size);
	return *this;
}

template<typename T, size_t Capacity>
inline fixcap_array<T, Capacity> &  fixcap_array<T, Capacity>::
	operator =(fixcap_array<T, Capacity> && other)
{
	_staticAssertTrivialRelocate<T>();

	_assignInternal(std::true_type{}, other.data(), other._size);
	other._setEmptyIfNot(std::has_trivial_destructor<T>());
	return *this;
}

template<typename T, size_t Capacity> template<typename InputIterator>
inline InputIterator  fixcap_array<T, Capacity>::
	assign(InputIterator first, size_type count)
{
	if (Capacity >= count)
		return _assignInternal(can_memmove_ranges_with(data(), first), first, count);
	else
		throw _lengthExc();
}

template<typename T, size_t Capacity> template<typename InputRange>
inline void  fixcap_array<T, Capacity>::
	assign(const InputRange & range)
{
	_assign(_getSize(range, int{}), range);
}

template<typename T, size_t Capacity> template<typename InputIterator>
InputIterator  fixcap_array<T, Capacity>::
	append(InputIterator first, size_type count)
{
	if (_unusedCapacity() >= count)
		return _appendN(can_memmove_ranges_with(data(), first), first, count);
	else
		throw _lengthExc();
}

template<typename T, size_t Capacity> template<typename InputRange>
typename fixcap_array<T, Capacity>::iterator  fixcap_array<T, Capacity>::
	append(const InputRange & range)
{
	iterator const oldEnd = end();

	_append(_getSize(range, int{}), range);

	return oldEnd;
}

template<typename T, size_t Capacity> template<typename... Params>
void  fixcap_array<T, Capacity>::
	push_back(Params &&... args)
{
	if (Capacity > _size)
	{
		::new(data() + _size) T(std::forward<Params>(args)...); // direct-initialize, or value-initialize if no args
		++_size;
	}
	else
	{	throw _lengthExc();
	}
}

template<typename T, size_t Capacity> template<typename... Params>
typename fixcap_array<T, Capacity>::iterator  fixcap_array<T, Capacity>::
	insert(const_iterator pos, Params &&... args)
{
	_staticAssertTrivialRelocate<T>();
	static_assert(std::is_nothrow_move_constructible<T>::value, "T must have noexcept move constructor");

	MEM_BOUND_ASSERT(begin() <= pos && pos <= end());

	if (Capacity > _size)
	{
		auto const posPtr = const_cast<pointer>(to_ptr(pos));
		size_type const nAfter = (data() + _size) - posPtr;
		auto tmp = T(std::forward<Params>(args)...);
		// Made a temporary in case source is in the destination range of the following memmove
		memmove(posPtr + 1, posPtr, nAfter * sizeof(T));
		++_size;
		::new(posPtr) T(std::move(tmp));

		return OEP_FIXCAPARR_ITERATOR(posPtr);
	}
	else
	{	throw _lengthExc();
	}
}

template<typename T, size_t Capacity>
inline void  fixcap_array<T, Capacity>::
	pop_back() NOEXCEPT
{
	MEM_BOUND_ASSERT(0 < _size);
	--_size;
	data()[_size].~T();
}

template<typename T, size_t Capacity>
void  fixcap_array<T, Capacity>::
	resize(size_type newSize)
{
	if (Capacity >= newSize)
	{
		_resizeImpl( newSize,
			[](pointer first, pointer last) { oetl::uninitialized_fill_default(first, last); } );
	}
	else
	{	throw _lengthExc();
	}
}

template<typename T, size_t Capacity>
void  fixcap_array<T, Capacity>::
	resize(size_type newSize, const T & addVal)
{
	if (Capacity >= newSize)
	{
		_resizeImpl( newSize,
			[&addVal](pointer first, pointer last)
			{
				std::uninitialized_fill(first, last, addVal);
			} );
	}
	else
	{	throw _lengthExc();
	}
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::iterator  fixcap_array<T, Capacity>::
	erase(iterator pos)
{
	_staticAssertTrivialRelocate<T>();

	(*pos).~T();
	pointer const next = to_ptr(pos) + 1;
	memmove(to_ptr(pos), next, ((data() + _size) - next) * sizeof(T)); // move [pos + 1, _end) to [pos, _end - 1)
	--_size;
	return pos;
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::iterator  fixcap_array<T, Capacity>::
	erase(iterator first, iterator last)
{
	_staticAssertTrivialRelocate<T>();

	MEM_BOUND_ASSERT(to_ptr(first) <= to_ptr(last));
	if (first < last)
	{
		size_type const nAfterLast = (data() + _size) - to_ptr(last);
		_detail::Destroy(first, last);
		// move [last, _end) to [first, first + nAfterLast)
		memmove(to_ptr(first), to_ptr(last), nAfterLast * sizeof(T));
		_size -= to_ptr(last) - to_ptr(first);
	}
	return first;
}

template<typename T, size_t Capacity>
inline void  fixcap_array<T, Capacity>::
	erase_from(iterator newEnd) NOEXCEPT
{
	pointer const first = to_ptr(newEnd);
	MEM_BOUND_ASSERT(data() <= first && first <= data() + _size);
	_detail::Destroy(first, data() + _size);
	_size = first - data();
}

template<typename T, size_t Capacity>
inline void  fixcap_array<T, Capacity>::
	clear() NOEXCEPT
{
	erase_from(begin());
}

template<typename T, size_t Capacity>
inline bool  fixcap_array<T, Capacity>::
	empty() const NOEXCEPT
{
	return 0 == _size;
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::pointer  fixcap_array<T, Capacity>::
	data() NOEXCEPT
{
	return reinterpret_cast<pointer>(_data);
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::const_pointer  fixcap_array<T, Capacity>::
	data() const NOEXCEPT
{
	return reinterpret_cast<const_pointer>(_data);
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::iterator  fixcap_array<T, Capacity>::
	begin() NOEXCEPT
{
	return OEP_FIXCAPARR_ITERATOR(data());
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::const_iterator  fixcap_array<T, Capacity>::
	begin() const NOEXCEPT
{
	return OEP_FIXCAPARR_CONST_ITER(data());
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::iterator  fixcap_array<T, Capacity>::
	end() NOEXCEPT
{
	return OEP_FIXCAPARR_ITERATOR(data() + _size);
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::const_iterator  fixcap_array<T, Capacity>::
	end() const NOEXCEPT
{
	return OEP_FIXCAPARR_CONST_ITER(data() + _size);
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::reference  fixcap_array<T, Capacity>::
	front() NOEXCEPT
{
	return (*this)[0];
}
template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::const_reference  fixcap_array<T, Capacity>::
	front() const NOEXCEPT
{
	return (*this)[0];
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::reference  fixcap_array<T, Capacity>::
	back() NOEXCEPT
{
	return (*this)[_size - 1];
}
template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::const_reference  fixcap_array<T, Capacity>::
	back() const NOEXCEPT
{
	return (*this)[_size - 1];
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::reference  fixcap_array<T, Capacity>::
	at(size_type index)
{
	if (_size > index)
		return data()[index];
	else
		throw out_of_range("Invalid index fixcap_array::at");
}
template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::const_reference  fixcap_array<T, Capacity>::
	at(size_type index) const
{
	if (_size > index)
		return data()[index];
	else
		throw out_of_range("Invalid index fixcap_array::at");
}

template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::reference  fixcap_array<T, Capacity>::
	operator[](size_type index) NOEXCEPT
{
	MEM_BOUND_ASSERT(_size > index);
	return data()[index];
}
template<typename T, size_t Capacity>
inline typename fixcap_array<T, Capacity>::const_reference  fixcap_array<T, Capacity>::
	operator[](size_type index) const NOEXCEPT
{
	MEM_BOUND_ASSERT(_size > index);
	return data()[index];
}

#undef OEP_FIXCAPARR_ITERATOR
#undef OEP_FIXCAPARR_CONST_ITER

} // namespace oetl

template<typename T, size_t C>
inline typename oetl::fixcap_array<T, C>::iterator  oetl::
	erase_unordered(fixcap_array<T, C> & ctr, typename fixcap_array<T, C>::iterator pos)
{
	*pos = std::move(ctr.back());
	ctr.pop_back();
	return pos;
}

template<typename T, size_t Capacity>
inline bool  oetl::operator==
	(const fixcap_array<T, Capacity> & left, const fixcap_array<T, Capacity> & right)
{
	return left.size() == right.size() &&
		   std::equal(left.begin(), left.end(), right.begin());
}
