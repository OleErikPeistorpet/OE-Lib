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
		Iterator first;
		Iterator last;

		iterator_range(Iterator first, Iterator last)  : first(first), last(last) {}

		Iterator begin() const  { return first; }
		Iterator end() const    { return last; }
	};
#endif

/// Create an iterator_range from two iterators, with type deduced from arguments
template<typename Iterator> inline
iterator_range<Iterator> make_view(Iterator first, Iterator last)  { return {first, last}; }


/// Wrapper for iterator and size. Similar to gsl::span, less safe, but not just for arrays
template< typename Iterator,
          bool = std::is_base_of<std::random_access_iterator_tag,
                                 typename std::iterator_traits<Iterator>::iterator_category>::value >
class counted_view
{
public:
	using iterator        = Iterator;
	using value_type      = typename std::iterator_traits<Iterator>::value_type;
	using difference_type = typename std::iterator_traits<Iterator>::difference_type;
	using size_type       = size_t;  // difference_type would be OK

	/// Initialize to empty
	counted_view()                                 : _size() {}
	counted_view(iterator first, size_type count)  : _begin(first), _size(count) {}
	/// Construct from array or container with matching iterator type
	template<typename SizedRange>
	counted_view(SizedRange & r)  : _begin(::adl_begin(r)), _size(oel::ssize(r)) {}

	size_type size() const   { return _size; }

	iterator  begin() const  { return _begin; }

	/// Increment begin, decrementing size
	void      drop_front()  { OEL_ASSERT_MEM_BOUND(_size > 0);  ++_begin; --_size; }
	/// Decrement end
	void      drop_back()   { OEL_ASSERT_MEM_BOUND(_size > 0);  --_size; }

protected:
	Iterator  _begin;
	size_type _size;
};

/// Specialization for random-access Iterator
template<typename Iterator>
class counted_view<Iterator, true> : public counted_view<Iterator, false>
{
	using _base = counted_view<Iterator, false>;

public:
	using typename _base::iterator;
	using typename _base::value_type;
	using typename _base::difference_type;
	using typename _base::size_type;

	counted_view() = default;
	counted_view(iterator first, size_type count)  : _base(first, count) {}
	template<typename SizedRange>
	counted_view(SizedRange & r)                   : _base(::adl_begin(r), oel::ssize(r)) {}

	iterator  end() const   { return this->_begin + this->_size; }

	typename std::iterator_traits<Iterator>::reference
		operator[](difference_type index) const  { return this->_begin[index]; }

	/// Return plain pointer to underlying array. Will only be found with contiguous Iterator (see to_pointer_contiguous)
	template<typename It1 = Iterator>
	auto data() const
	 -> decltype( to_pointer_contiguous(std::declval<It1>()) )  { return to_pointer_contiguous(this->_begin); }
};

/// Create a counted_view from iterator and count, with type deduced from first
template<typename Iterator> inline
counted_view<Iterator> make_view_n(Iterator first, typename counted_view<Iterator>::size_type count)
	{ return {first, count}; }


/// Create an iterator_range of move_iterator from two iterators
template<typename InputIterator> inline
iterator_range< std::move_iterator<InputIterator> >
	move_range(InputIterator first, InputIterator last)  { using MoveIt = std::move_iterator<InputIterator>;
	                                                       return {MoveIt{first}, MoveIt{last}}; }

/// Create a counted_view of move_iterator from iterator and count
template<typename InputIterator> inline
counted_view< std::move_iterator<InputIterator> >
	move_range_n(InputIterator first, typename counted_view< std::move_iterator<InputIterator> >::size_type count)
	{ return {std::make_move_iterator(first), count}; }

/// Create a counted_view of move_iterator from reference to an array or container
template<typename Container> inline
auto move_range(Container & c)
 -> counted_view< std::move_iterator<decltype(begin(c))> >  { return oel::move_range_n(begin(c), oel::ssize(c)); }
/// Create a counted_view of move_iterator from counted_view
template<typename InputIterator> inline
counted_view< std::move_iterator<InputIterator> >
	move_range(counted_view<InputIterator> v)  { return {std::make_move_iterator(v.begin()), v.size()}; }
/// Create an iterator_range of move_iterator from iterator_range
template<typename InputIterator> inline
iterator_range< std::move_iterator<InputIterator> >
	move_range(iterator_range<InputIterator> r)  { return oel::move_range(r.begin(), r.end()); }

} // namespace oel
