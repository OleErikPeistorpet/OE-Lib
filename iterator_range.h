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
	struct iterator_range
	{
		Iterator first;
		Iterator last;

		Iterator begin() const  { return first; }
		Iterator end() const    { return last; }
	};
#endif


/// Create a std::move_iterator from InputIterator
template<typename InputIterator> inline
std::move_iterator<InputIterator> make_move_iter(InputIterator it)  { return std::move_iterator<InputIterator>(it); }


/// Create an iterator_range from two iterators
template<typename Iterator> inline
iterator_range<Iterator> make_range(Iterator first, Iterator last)  { return {first, last}; }

/// Create an iterator_range of move_iterator from range reference (array or STL container)
template<typename InputRange>
auto move_range(InputRange & range) -> iterator_range< std::move_iterator<decltype(begin(range))> >;
/// Create an iterator_range of move_iterator from iterator_range
template<typename InputIterator>
iterator_range< std::move_iterator<InputIterator> >  move_range(const iterator_range<InputIterator> & range);
/// Create an iterator_range of move_iterator from two iterators
template<typename InputIterator>
iterator_range< std::move_iterator<InputIterator> >  move_range(InputIterator first, InputIterator last);

} // namespace oel



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


template<typename InputRange>
inline auto oel::move_range(InputRange & r) -> iterator_range< std::move_iterator<decltype(begin(r))> >
{
	return oel::make_range( make_move_iter(begin(r)), make_move_iter(end(r)) );
}

/// Create an iterator_range of move_iterator from iterator_range
template<typename InputIterator>
inline oel::iterator_range< std::move_iterator<InputIterator> >  oel::
	move_range(const iterator_range<InputIterator> & r)
{
	return oel::make_range( make_move_iter(r.begin()), make_move_iter(r.end()) );
}

/// Create an iterator_range of move_iterator from two iterators
template<typename InputIterator>
inline oel::iterator_range< std::move_iterator<InputIterator> >  oel::
	move_range(InputIterator first, InputIterator last)
{
	return oel::make_range(make_move_iter(first), make_move_iter(last));
}