#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/algo_detail.h"
#include "auxi/dynarray_iterator.h"
#include "align_allocator.h"


namespace oel
{

template< typename T >
true_type specify_trivial_relocate(array<T>);

/**
* @brief A sequence container that encapsulates arrays with a size that is fixed at construction and
*	does not change throughout the lifetime of the object, except when doing swap or move assignment.
*
* The elements are stored contiguously, which means that elements can be accessed not only through iterators, but also using
* offsets on regular pointers to elements. */
template< typename T >
class [[clang::trivial_abi]] array
{
public:
	using value_type      = T;
	using difference_type = ptrdiff_t;
	using size_type       = size_t;

#if OEL_MEM_BOUND_DEBUG_LVL
	using iterator       = debug::dynarray_iterator<T *, T>;
	using const_iterator = debug::dynarray_iterator<const T *, T>;
#else
	using iterator       = T *;
	using const_iterator = const T *;
#endif


	constexpr array() noexcept            : _data(), _end() {}
	array(size_type size, default_init_t);
	explicit array(size_type size);

	template< typename SizedInputRange,
	          typename /*EnableIfRange*/ = iterator_t<SizedInputRange> >
	explicit array(const SizedInputRange & r)   { _init(r); }

	array(array && other) noexcept;
	array(const array & other)              { _init(other); }

	array & operator =(array && other) & noexcept   { swap(*this, other);  return *this; }

	array & operator =(const array &) = delete;

	~array() noexcept;

	friend void    swap(array & a, array & b) noexcept   { swap(a._data, b._data);
	                                                       std::swap(a._end, b._end); }
	bool           empty() const noexcept   { return _data.get() == _end; }

	size_type      size() const noexcept    { return _end - _data.get(); }

	iterator       begin() noexcept          OEL_ALWAYS_INLINE { return _makeIterator{}(_data.get(), _data.get(), this); }
	const_iterator begin() const noexcept    OEL_ALWAYS_INLINE { return _makeConstIter{}(_data.get(), _data.get(), this); }
	const_iterator cbegin() const noexcept   OEL_ALWAYS_INLINE { return begin(); }

	iterator       end() noexcept          OEL_ALWAYS_INLINE { return _makeIterator{}(_end, _data.get(), this); }
	const_iterator end() const noexcept    OEL_ALWAYS_INLINE { return _makeConstIter{}(_end, _data.get(), this); }
	const_iterator cend() const noexcept   OEL_ALWAYS_INLINE { return end(); }

	T *       data() noexcept              OEL_ALWAYS_INLINE { return _data.get(); }
	const T * data() const noexcept        OEL_ALWAYS_INLINE { return _data.get(); }

	T &       operator[](size_type index) noexcept(nodebug)        { OEL_ASSERT(index < size());
	                                                                 return _data[index]; }
	const T & operator[](size_type index) const noexcept(nodebug)  { OEL_ASSERT(index < size());
	                                                                 return _data[index]; }


////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
	using _allocateWrap = _detail::DebugAllocateWrapper<allocator<T>, T *>;
	using _makeIterator = _detail::MakeDynarrayIterator<T *>;
	using _makeConstIter = _detail::MakeDynarrayIterator<const T *>;


	static T * _allocate(size_type n)
	{
		allocator<T> a;
		return _allocateWrap::Allocate(a, n);
	}

	struct _dealloc
	{	void operator()(T * p)
		{
			allocator<T> a;
			_allocateWrap::Deallocate(a, p, 0); // 0 would be dangerous with unknown allocator
		}
	};

	std::unique_ptr<T[], _dealloc> _data;
	T * _end;


	template< typename SizedInputRange >
	void _init(const SizedInputRange & src)
	{
		size_type const n = oel::ssize(src);
		_data.reset(_allocate(n));
		_end = _data.get() + n;

		struct {} a;
		_detail::UninitCopy(oel::adl_begin(src), _data.get(), _end, a);
	}
};

template< typename T >
array<T>::array(size_type size, default_init_t)
 :	_data(_allocate(size)),
	_end(_data.get() + size)
{
	struct {} a;
	_detail::UninitDefaultConstruct(_data.get(), _end, a);
}

template< typename T >
array<T>::array(size_type size)
 :	_data(_allocate(size)),
	_end(_data.get() + size)
{
	struct A {} a;
	_detail::UninitFill<A>{}(_data.get(), _end, a);
}

template< typename T >
inline array<T>::array(array && other) noexcept
 :	_data(std::move(other._data)),
	_end(other._end)
{
	other._data = nullptr;
	other._end = nullptr;
}

template< typename T >
array<T>::~array() noexcept
{
	_detail::Destroy(_data.get(), _end);
}

} // namespace oel
