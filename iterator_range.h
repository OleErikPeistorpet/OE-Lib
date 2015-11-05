#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"

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
		iterator_range(Iterator first, Iterator last)  : first(first), last(last) {}

		Iterator begin() const  { return first; }
		Iterator end() const    { return last; }

	protected:
		Iterator first;
		Iterator last;
	};
#endif

/// Create an iterator_range from two iterators. Intended for various dynarray functions
template<typename Iterator> inline
iterator_range<Iterator> make_range(Iterator first, Iterator last)  { return {first, last}; }

/// Wrapper for iterator and size. Similar to gsl::span, less safe, but not just for arrays
template<typename Iterator>
class counted_view
{
public:
	using iterator        = Iterator;
	using value_type      = typename std::iterator_traits<Iterator>::value_type;
	using difference_type = typename std::iterator_traits<Iterator>::difference_type;

	/// Initialize to empty
	counted_view()                              : count() {}
	counted_view(iterator first, size_t count)  : first(first), count(count) {}
	template<typename Container>
	counted_view(Container & c)                 : first(::adl_begin(c)), count(oel::ssize(c)) {}

	size_t   size() const  { return count; }

	iterator begin() const  { return first; }
	/// Only for random-access Iterator
	iterator end() const    { return first + count; }

	/// Only for random-access Iterator
	template<typename Integer>
	auto operator[](Integer index) const -> decltype(first[index])  { return first[index]; }

protected:
	Iterator first;
	size_t   count;
};

/// Create a counted_view from iterator and size
template<typename Iterator> inline
counted_view<Iterator>   as_counted_view(Iterator first, size_t count)  { return {first, count}; }

/// Create an iterator_range of move_iterator from two iterators
template<typename InputIterator>
iterator_range< std::move_iterator<InputIterator> >  move_range(InputIterator first, InputIterator last);
/// Create an iterator_range of move_iterator from reference to a range (array or container)
template<typename InputRange> inline
auto move_range(InputRange & r)
 -> iterator_range< std::move_iterator<decltype(begin(r))> >  { return oel::move_range(begin(r), end(r)); }
/// Create an iterator_range of move_iterator from iterator_range
template<typename InputIterator> inline
iterator_range< std::move_iterator<InputIterator> >  move_range(const iterator_range<InputIterator> & r)
	{ return oel::move_range(r.begin(), r.end()); }
/// Create a counted_view of move_iterator from counted_view
template<typename InputIterator> inline
counted_view< std::move_iterator<InputIterator> >    move_range(const counted_view<InputIterator> & r)
	{ return {std::make_move_iterator(r.begin()), r.size()}; }
/// Create a counted_view of move_iterator from iterator and size
template<typename InputIterator>
counted_view< std::move_iterator<InputIterator> >    move_range_n(InputIterator first, size_t count)
	{ return {std::make_move_iterator(first), count}; }

} // namespace oel



////////////////////////////////////////////////////////////////////////////////
//
// Implementation


template<typename InputIterator>
inline oel::iterator_range< std::move_iterator<InputIterator> >  oel::move_range(InputIterator first, InputIterator last)
{
	using MoveIt = std::move_iterator<InputIterator>;
	return {MoveIt{first}, MoveIt{last}};
}