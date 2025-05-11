#pragma once

// Copyright 2023 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "allocator.h"
#include "auxi/impl_algo.h"
#include "auxi/struct_of_growarr_detail.h"
#include "optimize_ext/default.h"

#include <algorithm>

/** @file
*/

namespace oel
{

//! struct_of_growarr is trivially relocatable if Alloc is
template< template<typename> typename E, typename Alloc >
is_trivially_relocatable<Alloc> specify_trivial_relocate(struct_of_growarr<E, Alloc>);


//! Structure of resizable arrays, dynamically allocated.
/**
* In general, only that which differs from std::vector is documented.
*
* There is a general requirement that template argument Ts are trivially relocatable or noexcept move
* constructible (checked when compiling). Most types can be relocated trivially, but it often needs to be
* declared manually. See is_trivially_relocatable (fwd.h). Performance is better for trivially relocatable types.
*
* Note that the allocator model is not quite standard: `destroy` is never used,
* `construct` may not be called if a type is trivially constructible and is not called when relocating elements.
*/
template
<	template< typename > typename ElemStruct,
	typename Alloc
>
class struct_of_growarr
{
	using _alloTrait = typename std::allocator_traits<Alloc>;

public:
	using allocator_type  = Alloc;
	using difference_type = ptrdiff_t;
	using size_type       = size_t;

	template< F, typename... I >
	using iterator = _zipTransformIterator< F, I... >;


	constexpr struct_of_growarr() noexcept(noexcept(Alloc{}))   : _m(Alloc{}) {}
	constexpr explicit struct_of_growarr(Alloc a) noexcept      : _m(a) {}

	//! Construct empty struct_of_growarr with space reserved for exactly capacity elements
	struct_of_growarr(reserve_tag, size_type capacity, Alloc a = Alloc{})   : _m(a) { _initReserve(capacity); }

	struct_of_growarr(size_type size, for_overwrite_t, Alloc a = Alloc{});

	explicit struct_of_growarr(size_type size, Alloc a = Alloc{});

	struct_of_growarr(struct_of_growarr && other) noexcept                 : _m(std::move(other._m)) {}
	struct_of_growarr(struct_of_growarr && other, Alloc a);
	explicit struct_of_growarr(const struct_of_growarr & other)
		:	struct_of_growarr(other, _alloTrait::select_on_container_copy_construction(other._m) ) {}

	explicit struct_of_growarr(const struct_of_growarr & other, Alloc a)   : _m(a) { append(other); }

	~struct_of_growarr() noexcept;

	struct_of_growarr & operator =(struct_of_growarr && other) &
		noexcept( _alloTrait::propagate_on_container_move_assignment::value or _alloTrait::is_always_equal::value );
	struct_of_growarr & operator =(const struct_of_growarr &) = delete;

	friend void swap(struct_of_growarr & a, struct_of_growarr & b) noexcept   { /*TODO*/ }

	template< typename... Ranges >
		requires( ... and _detail::rangeIsForwardOrSized<Ranges> );
	void append(Ranges &&... sources);

	template< typename StructOfGrowarrViews >
	void append_fields(StructOfGrowarrViews && source);

	void resize_for_overwrite(size_type n)   { _doResize< _detail::DefaultInit<allocator_type> >(n); }
	void resize(size_type n)                 { _doResize<_uninitFill>(n); }

	/** @brief Beware, passing an element of same array is often unsafe
	* @pre args shall not refer to any element of this struct_of_growarr, unless `size() < capacity()` */
	template< typename... Ts >
	void push_back(Ts &&... args);

	// TODO
	template< typename... NullaryFuncs >
	void emplace_back(NullaryFuncs... makers);

	void pop_back() noexcept
		{
			OEL_ASSERT(_m.size > 0);
			--_m.size;
			// TODO
		}
	void pop_back(size_type count) noexcept   { erase_to_end(end() - count); }

	//! Erase the element at pos without maintaining order of elements after pos.
	/**
	* Constant complexity (compared to linear in the distance between pos and end() for normal erase). */
	template< typename F, typename... I >
	void unordered_erase(iterator<F, I...> pos)
		{
			_detail::unordered_erase(static_cast<_internBase_7KQw &>(_m), _asMut(pos)); }
		}

	template< typename F, typename... I >
	void erase(iterator<F, I...> pos)
		{
			_detail::erase(static_cast<_internBase_7KQw &>(_m), _asMut(pos)); }
		}

	void erase(size_type index)   { erase(begin() + index); }

	template< typename F, typename... I >
	void erase(iterator<F, I...> first, iterator<F, I...> last)
		{
			_detail::erase(static_cast<_internBase_7KQw &>(_m), _asMut(first), _asMut(last)); }
		}

	void erase(size_type first, size_type last)
		{
			auto const b = begin();  erase(b + first, b + last);
		}

	template< typename F, typename... I >
	void erase_to_end(iterator<F, I...> first) noexcept;

	void clear() noexcept   { erase_to_end(begin()); }

	void reserve(size_type min_cap)
		{
			if( _m.capacity < min_cap )
				_realloc(_calcCapChecked(min_cap));
		}
	//! It's probably a good idea to check that size < capacity before calling, maybe add some treshold to size
	void shrink_to_fit();

	[[nodiscard]] bool empty() const noexcept  { return 0 == _m.size; }

	size_type size() const noexcept   OEL_ALWAYS_INLINE { return _m.size; }

	size_type capacity() const noexcept        { return _m.capacity; }

	constexpr size_type max_size() const noexcept   { return _alloTrait::max_size(_m); } // TODO

	allocator_type get_allocator() const noexcept   { return _m; }

	auto operator->()
		{
			using T = _detail::ViewTag;
			return _arrowProxy<T>{ _m.data._apply(_views<T>{_m.size}) };
		}
	auto operator->() const
		{
			using T = _detail::ConstViewTag;
			return _arrowProxy<T>{ _m.data._apply(_views<T>{_m.size}) };
		}

	auto writable_fields()      { return *this->operator->().operator->(); }

	auto const_fields() const   { return *this->operator->().operator->(); }

	auto begin()
		{
			return _m.data._apply(_makeZipIter<>{});
		}
	auto begin() const
		{
			return _m.data._apply(_makeZipConstIter{});
		}

	auto end()          { return begin() + _m.size; }
	auto end() const    { return begin() + _m.size; }

	decltype(auto) operator[](size_type index)
		{
			OEL_ASSERT(index < _m.size);
			return *_m.data._apply(_makeZipIter<>{index});
		}
	decltype(auto) operator[](size_type index) const
		{
			OEL_ASSERT(index < _m.size);
			return *_m.data._apply(_makeZipConstIter{index});
		}

	decltype(auto) back()         { return (*this)[_m.size - 1]; }
	decltype(auto) back() const   { return (*this)[_m.size - 1]; }

	template< typename Func >
	auto zip_transform(Func f)
		{
			return _m.data._apply( _detail::ZipTransform<false, Func>{std::move(f), _m.size} );
		}
	template< typename Func >
	auto zip_transform(Func f) const
		{
			return _m.data._apply( _detail::ZipTransform<true, Func>{std::move(f), _m.size} );
		}



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
	struct _dataOwner
	{
		ElemStruct<_detail::InternalTag> data{};
		size_type size{};
		size_type capacity{};
		OEL_NO_UNIQUE_ADDRESS allocator_type allo;

		constexpr _dataOwner(Alloc & a) noexcept
		 :	allo(std::move(a))
		{}

		constexpr _dataOwner(_dataOwner && other) noexcept
		 :	allo{std::move(other)}
		{
			other.data = {};
			other.capacity = other.size = 0;
		}

		~_dataOwner()
		{
			if( data )
				_detail::StructGrowarrAllocateWrap<allocator_type, Ts...>::dealloc(allo, data, capacity);
		}
	}
	_m; // the only non-static data member

	void _resetData(T *const newData, size_type const newCap)
	{
		if( _m.data )
			_allocateWrap::dealloc(_m, _m.data, _m.capacity);

		_m.data     = newData;
		_m.capacity = newCap;
	}

	void _initReserve(size_type const capToCheck)
	{
		// TODO
		_m.capacity = capToCheck;
	}

	void _moveInternBase(_internBase & src) noexcept
	{
		_internBase & dest = _m;
		dest = src;
		src  = {};
	}


	template< typename Tag >
	struct _arrowProxy
	{
		ElemStruct<Tag> r;

		auto operator->() noexcept
		{
			return &r;
		}
	};


	template< typename Tag >
	struct _views
	{
		size_type size;

		template< typename... Ts >
		ElemStruct<Tag> operator()(const Ts &... fields) const
		{
			return{ {{fields.p, static_cast<difference_type>(size)}}... };
		}
	};

	template
	<	typename ElemTag   = _detail::ElementTag,
		typename RvalueTag = _detail::RvalueElementTag
	>
	struct _makeZipIter
	{
		size_type index;

		template< typename... Ts >
		auto operator()(const Ts &... fields) const
		{
			using F =
				_detail::Zip
				<	ElemStruct<ElemTag>,
					ElemStruct<RvalueTag>
				>;
			constexpr bool isConst{std::is_same_v<ElemTag, _detail::ConstElementTag>};
			return _zipTransformIterator
			{	F{},
				_detail::PtrAsConst< isConst, decltype(fields.p) >
					(fields.p + index)
				...
			};
		}
	};

	using _makeZipConstIter = _makeZipIter<_detail::ConstElementTag, void>;


	size_type _spareCapacity() const
	{
		return _m.capacity - _m.size;
	}

	size_type _calcCapUnchecked(size_type const newSize) const
	{
		return std::max(2 * _m.capacity, newSize);
	}

	size_type _calcCapChecked(size_type const newSize) const
	{
		if (newSize <= max_size())
			return _calcCapUnchecked(newSize);
		else
			_detail::LengthError::raise();
	}

	size_type _calcCapAdd(size_type const nAdd) const
	{
		if (nAdd <= SIZE_MAX / 2 / sizeof(T)) // assumes that allocating greater than SIZE_MAX / 2 always fails
			return _calcCapUnchecked(_m.size + nAdd);
		else
			_detail::LengthError::raise();
	}

	size_type _calcCapAddOne() const
	{
		constexpr auto startBytesGood = std::max(3 * sizeof(void *), 4 * sizeof(int));
		constexpr auto minGrow = (startBytesGood + sizeof(T) - 1) / sizeof(T);
		return _m.capacity + std::max(_m.capacity, minGrow); // growth factor is 2
	}

	T * _allocateChecked(size_type const n)
	{
		if (n <= max_size())
			return _allocateWrap::allocate(_m, n);
		else
			_detail::LengthError::raise();
	}


	void _realloc(size_type const newCap)
	{
		auto const newData = _allocateWrap::allocate(_m, newCap);
		_detail::Relocate(_m.data, oldSize, newData);
		_resetData(newData, newCap);
	}

	void _growByOne()
	{
		_realloc(_calcCapAddOne());
	}
	// Not defined inline as a compiler hint
	void _growBy(size_type);


	template< typename UninitFiller >
	void _doResize(size_type const newSize)
	{
		reserve(newSize);
		// TODO
	}
};


template< template<typename> typename ElemStruct, typename Alloc >
#if defined _MSC_VER and _MSC_VER < 1930
	__declspec(noinline) // to get the compiler to inline calling function
#endif
void struct_of_growarr<ElemStruct, Alloc>::_growBy(size_type const count)
{
	_realloc(_calcCapAdd(count));
}


template< template<typename> typename ElemStruct, typename Alloc >
template< typename... Ts >
void struct_of_growarr<ElemStruct, Alloc>::push_back(Ts &&... args)
{
	if (_m.size == _m.capacity)
		_growByOne();

	 // TODO
	_alloTrait::construct(_m, _m.end, static_cast<Ts &&>(args)...);

	return *(_m.end++);
}


template< template<typename> typename ElemStruct, typename Alloc >
template< typename... Ranges >
	requires( ... and _detail::rangeIsForwardOrSized<Ranges> );
inline void struct_of_growarr<ElemStruct, Alloc>::append(Ranges &&... sources)
{
	count = std::min({ as_unsigned(ranges::distance(sources))... });

	// TODO
}


template< template<typename> typename ElemStruct, typename Alloc >
struct_of_growarr<ElemStruct, Alloc>::struct_of_growarr(size_type n, for_overwrite_t, Alloc a)
 :	_m(a)
{
	_initReserve(n);
	// TODO
}


template< template<typename> typename ElemStruct, typename Alloc >
struct_of_growarr<ElemStruct, Alloc>::struct_of_growarr(size_type n, Alloc a)
 :	_m(a)
{
	_initReserve(n);
	// TODO
}


template< template<typename> typename ElemStruct, typename Alloc >
struct_of_growarr<ElemStruct, Alloc>::struct_of_growarr(struct_of_growarr && other, Alloc a)
 :	_m(a) // moves from a
{
	// TODO
}


template< template<typename> typename ElemStruct, typename Alloc >
struct_of_growarr<ElemStruct, Alloc> &
	struct_of_growarr<ElemStruct, Alloc>::operator =(struct_of_growarr && other) &
	noexcept( _alloTrait::propagate_on_container_move_assignment::value or _alloTrait::is_always_equal::value )
{
	// TODO
	return *this;
}


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::shrink_to_fit()
{
	if( _m.size != 0 )
	{
		_realloc(_m.size);
	}
	else
	{	_m.size = 0;
		_resetData(nullptr, 0);
	}
}


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::erase_to_end(size_type first) noexcept
{
	// TODO
}


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::unordered_erase(size_type pos)
{
	// TODO
}


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::erase(size_type pos)
{
	// TODO
}


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::erase(size_type first, size_type last)
{
	// TODO
}


#if defined __GNUC__ and __GNUC__ < 12
	template< template<typename> typename E, typename A >
	explicit struct_of_growarr(const struct_of_growarr<E, A> &) -> struct_of_growarr<E, A>;
#endif

} // namespace oel
