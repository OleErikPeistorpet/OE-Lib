#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#ifndef OEL_NO_BOOST
	#include <boost/range/iterator_range_core.hpp>
#endif


namespace oel
{

#ifndef OEL_NO_BOOST
	using boost::iterator_range;
#else
	template<typename Iterator>
	class iterator_range
	{
	public:
		iterator_range(Iterator first, Iterator last)  : _first(first), _last(last) {}

		Iterator begin() const  { return _first; }
		Iterator end() const    { return _last; }

	protected:
		Iterator _first;
		Iterator _last;
	};
#endif

/// Create an iterator_range from two iterators, with type deduced from arguments
template<typename Iterator> inline
iterator_range<Iterator> as_view(Iterator first, Iterator last)  { return {first, last}; }


/// Wrapper for iterator and size. Similar to gsl::span, less safe, but not just for arrays
template<typename Iterator>
class counted_view
{
public:
	using iterator        = Iterator;
	using value_type      = typename std::iterator_traits<Iterator>::value_type;
	using reference       = typename std::iterator_traits<Iterator>::reference;
	using difference_type = typename std::iterator_traits<Iterator>::difference_type;
	using size_type       = size_t;  // difference_type would be OK

	/// Initialize to empty
	counted_view()                                 : _count() {}
	counted_view(iterator first, size_type count)  : _begin(first), _count(count) {}
	/// Construct from container with matching iterator type
	template<typename Container>
	counted_view(Container & c)  : _begin(::adl_begin(c)), _count(oel::ssize(c)) {}

	size_type size() const   { return _count; }

	bool      empty() const  { return 0 == _count; }

	iterator  begin() const  { return _begin; }
	/// Only for random-access Iterator
	iterator  end() const    { return _begin + _count; }

	reference front() const  { return *_begin; }
	/// Only for random-access Iterator
	reference back() const   { return *(end() - 1); }

	/// Will exist only with random-access Iterator (SFINAE)
	template<typename Integer>
	auto operator[](Integer index) const -> decltype(begin()[index])  { return _begin[index]; }

	/// Will exist only with contiguous Iterator (SFINAE)
	template<typename = Iterator>
	auto data() const -> decltype( to_pointer_contiguous(begin()) )  { return to_pointer_contiguous(_begin); }

	/// Increment begin, decrementing size
	void drop_front();
	/// Decrement end
	void drop_back();

protected:
	Iterator  _begin;
	size_type _count;
};

/// Create a counted_view from iterator and size, with type deduced from first
template<typename Iterator> inline
counted_view<Iterator> as_view_n(Iterator first, size_t count)  { return {first, count}; }


/// Create an iterator_range of move_iterator from two iterators
template<typename InputIterator>
iterator_range< std::move_iterator<InputIterator> >  move_range(InputIterator first, InputIterator last);

/// Create an iterator_range of move_iterator from reference to an array or container
template<typename Container> inline
auto move_range(Container & c)
 -> iterator_range< std::move_iterator<decltype(begin(c))> >  { return oel::move_range(begin(c), end(c)); }
/// Create an iterator_range of move_iterator from iterator_range
template<typename InputIterator> inline
iterator_range< std::move_iterator<InputIterator> >  move_range(const iterator_range<InputIterator> & r)
	{ return oel::move_range(r.begin(), r.end()); }
/// Create a counted_view of move_iterator from counted_view
template<typename InputIterator> inline
counted_view< std::move_iterator<InputIterator> >    move_range(const counted_view<InputIterator> & r)
	{ return {std::make_move_iterator(r.begin()), r.size()}; }

/// Create a counted_view of move_iterator from iterator and size
template<typename InputIterator> inline
counted_view< std::move_iterator<InputIterator> >    move_range_n(InputIterator first, size_t count)
	{ return {std::make_move_iterator(first), count}; }



////////////////////////////////////////////////////////////////////////////////
//
// Implementation


template<typename Iterator>
inline void counted_view<Iterator>::drop_front()
{
	OEL_ASSERT_MEM_BOUND(_count > 0);
	++_begin; --_count;
}

template<typename Iterator>
inline void counted_view<Iterator>::drop_back()
{
	OEL_ASSERT_MEM_BOUND(_count > 0);
	--_count;
}

} // namespace oel

template<typename InputIterator>
inline oel::iterator_range< std::move_iterator<InputIterator> >  oel::move_range(InputIterator first, InputIterator last)
{
	using MoveIt = std::move_iterator<InputIterator>;
	return {MoveIt{first}, MoveIt{last}};
}