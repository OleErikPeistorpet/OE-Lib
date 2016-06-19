#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/contiguous_iterator.h"
#include "compat/default.h"
#include "align_allocator.h"


namespace oel
{

namespace _detail
{
	template<typename, typename> struct FcaProxy;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, size_t C, typename S>
is_trivially_relocatable<T> specify_trivial_relocate(fixcap_array<T, C, S> &&);

/// Overloads generic erase_unordered(RandomAccessContainer &, RandomAccessContainer::size_type) (in util.h)
template<typename T, size_t C, typename S> inline
void erase_unordered(fixcap_array<T, C, S> & a,
                     typename fixcap_array<T, C, S>::size_type index)  { a.erase_unordered(a.begin() + index); }

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
	using pointer         = T *;
	using const_pointer   = const T *;
	using size_type       = Size;
	using difference_type = std::ptrdiff_t;

#if OEL_MEM_BOUND_DEBUG_LVL
	using iterator       = contiguous_ctnr_iterator< T *, _detail::FcaProxy<T, Size> >;
	using const_iterator = contiguous_ctnr_iterator< const T *, _detail::FcaProxy<T, Size> >;
#else
	using iterator       = pointer;
	using const_iterator = const_pointer;
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	fixcap_array() noexcept  : _size() {}

	/** @brief Elements are default initialized, meaning non-class T produces indeterminate values
	* @throw length_error if size > Capacity  */
	fixcap_array(size_type size, default_init_tag);
	/// Throws length_error if size > Capacity. (Elements are value-initialized, same as std::vector)
	explicit fixcap_array(size_type size);
	fixcap_array(size_type size, const T & fillVal); ///< Throws length_error if size > Capacity

	template<typename InputRange, typename /*EnableIfRange*/ = decltype( ::adl_cbegin(std::declval<InputRange>()) )>
	explicit fixcap_array(const InputRange & range)  : _size() { assign(range); }

	fixcap_array(std::initializer_list<T> init)  : _size() { assign(init); }

	fixcap_array(fixcap_array && other);
	fixcap_array(const fixcap_array & other)  { _uninitCopyFrom(is_trivially_copyable<T>(), other); }

	~fixcap_array() noexcept  { _detail::Destroy(data(), data() + _size); }

	fixcap_array & operator =(fixcap_array && other);
	fixcap_array & operator =(const fixcap_array & other);

	fixcap_array & operator =(std::initializer_list<T> il)  { assign(il);  return *this; }

	/**
	* @brief Replace the contents with source
	* @throw length_error if count > Capacity  */
	template<typename InputRange>
	auto      assign(const InputRange & source) -> decltype(::adl_begin(source))
																	{ return _assign(_getSize(source, 0), source); }
	void      assign(size_type count, const T & val)  { clear(); append(count, val); }

	/**
	* @brief Add at end the elements from range (in order)
	* @param source object which begin and end can be called on (an array, STL container or iterator_range)
	* @return iterator pointing to first element added, or end if range is empty
	*
	* Any previous past-the-end iterator will point to the first element added.
	* Strong exception safety, aka. commit or rollback semantics  */
	template<typename InputRange>
	iterator      append(const InputRange & source);
	/**
	* @brief Same as append(const InputRange &), except returning past-the-last of source
	* @return begin(source) incremented to end of source  */
	template<typename InputRange>
	auto      append_ret_src(const InputRange & source) -> decltype(::adl_begin(source))
																	{ return _append(_getSize(source, 0), source); }
	/// Equivalent to calling append(const InputRange &) with il as argument
	iterator      append(std::initializer_list<T> il)  { return append<>(il); }
	void          append(size_type count, const T & val);

	/**
	* @brief Added elements are default initialized, meaning non-class T produces indeterminate values
	* @throw length_error if count > Capacity  */
	void          resize(size_type count, default_init_tag)  {_resizeImpl(count, _detail::UninitFillDefaultA<T>); }
	/// Throws length_error if count > Capacity. (Value-initializes added elements, same as std::vector::resize)
	void          resize(size_type count)                    { _resizeImpl(count, _detail::UninitFillA<T>); }

	template<typename ForwardRange>
	iterator      insert_r(const_iterator pos, const ForwardRange & source);

	template<typename... Args>
	iterator      emplace(const_iterator pos, Args &&... elemInitArgs);  ///< Throws length_error when full

	iterator      insert(const_iterator pos, T && val)       { return emplace(pos, std::move(val)); }
	iterator      insert(const_iterator pos, const T & val)  { return emplace(pos, val); }

	template<typename... Args>
	void          emplace_back(Args &&... elemInitArgs);  ///< Throws length_error when full

	/// Throws length_error when full
	void          push_back(T && val)       { emplace_back(std::move(val)); }
	/// Throws length_error when full
	void          push_back(const T & val)  { emplace_back(val); }

	void          pop_back() OEL_NOEXCEPT_NDEBUG;

	iterator      erase_unordered(iterator pos)  { _eraseUnordered(pos, is_trivially_relocatable<T>());
	                                               return pos; }
	iterator      erase(iterator pos)            { _erase(pos, is_trivially_relocatable<T>());  return pos; }

	iterator      erase(iterator first, iterator last);
	/// Equivalent to erase(first, end()) (but potentially faster), making first the new end
	void          erase_to_end(iterator first) OEL_NOEXCEPT_NDEBUG;

	void          clear() noexcept        { erase_to_end(begin()); }

	bool          empty() const noexcept  { return 0 == _size; }

	bool          full() const noexcept   { return Capacity == _size; }

	size_type     size() const noexcept   { return _size; }

	static constexpr size_type max_size() noexcept  { return Capacity; }

	iterator       begin() noexcept         { return _makeIterator(_data); }
	const_iterator begin() const noexcept   { return _makeConstIter(_data); }
	const_iterator cbegin() const noexcept  { return _makeConstIter(_data); }

	iterator       end() noexcept           { return _makeIterator(&_data[_size]); }
	const_iterator end() const noexcept     { return _makeConstIter(&_data[_size]); }
	const_iterator cend() const noexcept    { return _makeConstIter(&_data[_size]); }

	reverse_iterator       rbegin() noexcept        { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept  { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() noexcept          { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const noexcept    { return const_reverse_iterator{begin()}; }

	T *            data() noexcept        { return reinterpret_cast<pointer>(_data); }
	const T *      data() const noexcept  { return reinterpret_cast<const_pointer>(_data); }

	reference       front() OEL_NOEXCEPT_NDEBUG        { return operator[](0); }
	const_reference front() const OEL_NOEXCEPT_NDEBUG  { return operator[](0); }

	reference       back() OEL_NOEXCEPT_NDEBUG         { return operator[](_size - 1); }
	const_reference back() const OEL_NOEXCEPT_NDEBUG   { return operator[](_size - 1); }

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) OEL_NOEXCEPT_NDEBUG;
	const_reference operator[](size_type index) const OEL_NOEXCEPT_NDEBUG;

	template<size_t C1, typename S1>
	friend bool operator==(const fixcap_array & left, const fixcap_array<T, C1, S1> & right)
	{
		return left.size() == right.size() &&
		       std::equal(left.begin(), left.end(), right.begin());
	}
	template<size_t C1, typename S1>
	friend bool operator!=(const fixcap_array & left, const fixcap_array<T, C1, S1> & right)  { return !(left == right); }


////////////////////////////////////////////////////////////////////////////////

// Implementation only in rest of the file


private:
	size_type _size;
	aligned_union_t<T> _data[Capacity];


	iterator _makeIterator(aligned_union_t<T> * pos)
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		return {reinterpret_cast<pointer>(pos), reinterpret_cast<const _detail::FcaProxy<T, Size> *>(this)};
	#else
		return reinterpret_cast<pointer>(pos);
	#endif
	}
	const_iterator _makeConstIter(const aligned_union_t<T> * pos) const
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		return {reinterpret_cast<const_pointer>(pos), reinterpret_cast<const _detail::FcaProxy<T, Size> *>(this)};
	#else
		return reinterpret_cast<const_pointer>(pos);
	#endif
	}

#if _MSC_VER
	__declspec(noreturn)
#endif
	static void _throwLenExc()
		#if !_MSC_VER
			__attribute__((noreturn))
		#endif
	{
		throw std::length_error("Not enough space in fixcap_array");
	}

	template<typename Range>
	static auto _getSize(const Range & r, int) -> decltype(static_cast<size_t>(oel::ssize(r))) { return oel::ssize(r); }

	template<typename Range>
	static std::false_type _getSize(const Range &, long) { return {}; }


	size_type _unusedCapacity() const
	{
		return Capacity - _size;
	}

	void _uninitCopyFrom(std::false_type, const fixcap_array<T, Capacity, Size> & src)
	{	// non-trivial copy
		_size = src._size;
		_detail::UninitCopyA(src.data(), data(), data() + src._size);
	}

	void _uninitCopyFrom(std::true_type, const fixcap_array<T, Capacity, Size> & src)
	{	// trivial copy
		//_size = src._size;
		//::memcpy(data(), src.data(), sizeof(T) * src._size);
		::memcpy(this, &src, sizeof(T) * sizeof src._size + src._size);
	}

	void _setEmptyIfNot(std::false_type)
	{
		_size = 0;
	}

	void _setEmptyIfNot(std::true_type) {}


	void _eraseUnordered(iterator const pos, true_type /*trivialRelocate*/)
	{
		OEL_ASSERT_MEM_BOUND((void *)pos._container == (void *)this);

		T & elem = *pos;
		elem.~T();
		--_size;
		auto &
	#if !_MSC_VER
			__attribute__((may_alias))
	#endif
			raw = reinterpret_cast<aligned_union_t<T> &>(elem);
		raw = reinterpret_cast<aligned_union_t<T> &>(data()[_size]); // relocate last element to pos
	}

	void _eraseUnordered(iterator pos, false_type)
	{
		*pos = std::move(back());
		pop_back();
	}

	void _erase(iterator pos, true_type)
	{
		pointer const ptr = to_pointer_contiguous(pos);
		OEL_ASSERT_MEM_BOUND(data() <= ptr && pos < end());

		ptr-> ~T();
		pointer const next = ptr + 1;
		size_type const nAfter = (data() + _size) - next;
		::memmove(ptr, next, sizeof(T) * nAfter); // move [pos + 1, _end) to [pos, _end - 1)
		--_size;
	}

	void _erase(iterator pos, false_type)
	{
		iterator last = std::move(pos + 1, end(), pos);
		(*last).~T();
		--_size;
	}


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
		{	_throwLenExc();
		}
	}

	template<typename CntigusIter>
	CntigusIter _assignInternal(std::true_type, CntigusIter first, size_type const count)
	{	// fastest assign
		_size = count;
		// Not portable for self assignment. Use memmove?
		::memcpy(data(), to_pointer_contiguous(first), sizeof(T) * count);

		return first + count;
	}

	template<typename InputIter>
	InputIter _assignInternal(std::false_type, InputIter src, size_type const count)
	{	// cannot use memcpy
		// FIXME: clean up
		auto copy = [](InputIter src, iterator dest, iterator dLast)
			{
				while (dest != dLast)
				{
					*dest = *src;
					++src; ++dest;
				}
				return src;
			};
		difference_type const diff = count - _size;
		if (0 < diff)
		{	// assign to old elements and construct rest
			src = copy(src, begin(), end());
			src = _detail::UninitCopyA(src, data() + _size, data() + count);
		}
		else
		{	// enough elements, copy new and destroy old
			src = copy(src, begin(), begin() + count);
			_detail::Destroy(data() + count, data() + _size);
		}
		_size = count;
		return src;
	}

	template<typename SizeRange>
	auto _assign(size_t count, const SizeRange & src) -> decltype(::adl_begin(src))
	{
		if (Capacity >= count)
		{
			auto first = ::adl_begin(src);
			return _assignInternal(can_memmove_with<pointer, decltype(first)>(), first, count);
		}
		else
		{	_throwLenExc();
		}
	}

	template<typename InputRange>
	auto _assign(std::false_type, const InputRange & src) -> decltype(::adl_begin(src))
	{	// no fast way of getting size
		clear();
		return append_ret_src(src);
	}

	template<typename CntigusIter>
	CntigusIter _appendInternal(std::true_type, CntigusIter first, size_type const count)
	{	// use memcpy
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (count > 0)
		{	// Dereference to catch out of range errors if the iterators have internal checks
			(void)*first;
			(void)*(first + (count - 1));
		}
	#endif
		::memcpy(data() + _size, to_pointer_contiguous(first), sizeof(T) * count);
		_size += count;

		return first + count;
	}

	template<typename InputIter>
	InputIter _appendInternal(std::false_type, InputIter src, size_type const count)
	{	// slower copy
		size_type newSize = _size + count;
		src = _detail::UninitCopyA(src, data() + _size, data() + newSize);
		_size = newSize;
		return src;
	}

	template<typename SizeRange>
	auto _append(size_t count, const SizeRange & src) -> decltype(::adl_begin(src))
	{
		if (_unusedCapacity() >= count)
		{
			auto first = ::adl_begin(src);
			return _appendInternal(can_memmove_with<pointer, decltype(first)>(), first, count);
		}
		else
		{	_throwLenExc();
		}
	}

	template<typename InputRange>
	auto _append(std::false_type, const InputRange & src) -> decltype(::adl_begin(src))
	{	// number of items unknown (slowest)
		auto f = ::adl_begin(src); auto l = ::adl_end(src);
		for (; f != l; ++f)
			emplace_back(*f);

		return f;
	}
};

template<typename T, size_t Capacity, typename Size>
fixcap_array<T, Capacity, Size>::fixcap_array(size_type size, default_init_tag)
{
	if (Capacity >= size)
	{
		_size = size;
		_detail::UninitFillDefaultA(data(), data() + size);
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
		_detail::UninitFillA(data(), data() + size);
	}
	else
	{	_throwLenExc();
	}
}

template<typename T, size_t Capacity, typename Size>
fixcap_array<T, Capacity, Size>::fixcap_array(size_type size, const T & val)
{
	if (Capacity >= size)
	{
		_size = size;
		_detail::UninitFillA(data(), data() + size, val);
	}
	else
	{	_throwLenExc();
	}
}

template<typename T, size_t Capacity, typename Size>
inline fixcap_array<T, Capacity, Size>::fixcap_array(fixcap_array && other)
{
	_detail::AssertTrivialRelocate<T>();

	_uninitCopyFrom(std::true_type{}, other);
	other._setEmptyIfNot(_detail::is_trivially_destructible<T>());
}

template<typename T, size_t Capacity>
fixcap_array<T, Capacity> &  fixcap_array<T, Capacity>::
	operator =(const fixcap_array & other)
{	// Bypassing Capacity check in _assign
	_assignInternal(is_trivially_copyable<T>(), other.data(), other._size);
	return *this;
}

template<typename T, size_t Capacity, typename Size>
inline fixcap_array<T, Capacity, Size> &  fixcap_array<T, Capacity, Size>::
	operator =(fixcap_array && other)
{
	_detail::AssertTrivialRelocate<T>();

	_assignInternal(std::true_type{}, other.data(), other._size);
	other._setEmptyIfNot(_detail::is_trivially_destructible<T>());
	return *this;
}

template<typename T, size_t Capacity, typename Size> template<typename InputRange>
typename fixcap_array<T, Capacity, Size>::iterator  fixcap_array<T, Capacity, Size>::
	append(const InputRange & src)
{
	iterator const oldEnd = end();

	_append(_getSize(src, int{}), src);

	return oldEnd;
}

template<typename T, size_t Capacity, typename Size>
void fixcap_array<T, Capacity, Size>::append(size_type n, const T & val)
{
	if (_unusedCapacity() >= n)
	{
		size_type newSize = _size + n;
		_detail::UninitFillA(data() + _size, data() + newSize, val);
		_size = newSize;
	}
	else
	{	_throwLenExc();
	}
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
	{	_throwLenExc();
	}
}

template<typename T, size_t Capacity, typename Size> template<typename... Args>
typename fixcap_array<T, Capacity, Size>::iterator  fixcap_array<T, Capacity, Size>::
	emplace(const_iterator pos, Args &&... args)
{
	_detail::AssertTrivialRelocate<T>();

	OEL_ASSERT_MEM_BOUND(begin() <= pos && pos <= end());

	if (Capacity > _size)
	{
		auto const pPos = const_cast<T *>(to_pointer_contiguous(pos));
		size_type const nAfterPos = (data() + _size) - pPos;
		// Temporary in case constructor throws or source is an element of this array at pos or after
		aligned_union_t<T> local;
		::new(&local) T(std::forward<Args>(args)...);
		// Move [pos, end) to [pos + 1, end + 1), conceptually destroying element at pos
		::memmove(pPos + 1, pPos, sizeof(T) * nAfterPos);
		++_size;

		::memcpy(pPos, &local, sizeof(T)); // relocate the new element to pos

		return _makeIterator(reinterpret_cast<aligned_union_t<T> *>(pPos));
	}
	else
	{	_throwLenExc();
	}
}

template<typename T, size_t Capacity, typename Size>
inline void fixcap_array<T, Capacity, Size>::pop_back() OEL_NOEXCEPT_NDEBUG
{
	OEL_ASSERT_MEM_BOUND(0 < _size);
	--_size;
	data()[_size].~T();
}

template<typename T, size_t Capacity, typename Size>
typename fixcap_array<T, Capacity, Size>::iterator  fixcap_array<T, Capacity, Size>::
	erase(iterator first, iterator last)
{
	_detail::AssertTrivialRelocate<T>();

	pointer const pFirst = to_pointer_contiguous(first);
	pointer const pLast = to_pointer_contiguous(last);
	OEL_ASSERT_MEM_BOUND(data() <= pFirst && pFirst <= pLast && last <= end());

	difference_type nErase = pLast - pFirst;
	if (0 < nErase)
	{
		_detail::Destroy(pFirst, pLast);
		size_type const nAfterLast = (data() + _size) - pLast;
		// move [last, _end) to [first, first + nAfterLast)
		::memmove(pFirst, pLast, sizeof(T) * nAfterLast);
		_size -= nErase;
	}
	return first;
}

template<typename T, size_t Capacity, typename Size>
void fixcap_array<T, Capacity, Size>::erase_to_end(iterator newEnd) OEL_NOEXCEPT_NDEBUG
{
	pointer const first = to_pointer_contiguous(newEnd);
	OEL_ASSERT_MEM_BOUND(data() <= first && first <= data() + _size);
	_detail::Destroy(first, data() + _size);
	_size = first - data();
}


template<typename T, size_t Capacity, typename Size>
inline T & fixcap_array<T, Capacity, Size>::at(size_type index)
{
	const auto & cSelf = *this;
	return const_cast<reference>(cSelf.at(index));
}
template<typename T, size_t Capacity, typename Size>
const T & fixcap_array<T, Capacity, Size>::at(size_type index) const
{
	if (static_cast<size_t>(index) < static_cast<size_t>(_size))
		return data()[index];
	else
		throw std::out_of_range("Invalid index fixcap_array::at");
}

template<typename T, size_t Capacity, typename Size>
inline T & fixcap_array<T, Capacity, Size>::operator[](size_type index) OEL_NOEXCEPT_NDEBUG
{
	OEL_ASSERT_MEM_BOUND(static_cast<size_t>(index) < static_cast<size_t>(_size));
	return data()[index];
}
template<typename T, size_t Capacity, typename Size>
inline const T & fixcap_array<T, Capacity, Size>::operator[](size_type index) const OEL_NOEXCEPT_NDEBUG
{
	OEL_ASSERT_MEM_BOUND(static_cast<size_t>(index) < static_cast<size_t>(_size));
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

		const T * Begin() const { return data; }
		const T * End() const   { return data + size; }
	};
}

} // namespace oel
