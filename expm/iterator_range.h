#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "util.h"

#include <boost/range/iterator_range_core.hpp>


namespace oetl
{

using boost::iterator_range;

/// Create an iterator_range from two iterators
template<typename Iterator> inline
iterator_range<Iterator> make_range(Iterator first, Iterator last)  { return {first, last}; }

/// Create an iterator_range of move_iterator from range reference
template<typename InputRange> inline
auto move_range(InputRange && range) -> iterator_range< std::move_iterator<decltype(begin(range))> >
{
	return oetl::make_range( make_move_iter(begin(range)), make_move_iter(end(range)) );
}

/// Create an iterator_range of move_iterator from two iterators
template<typename InputIterator> inline
iterator_range< std::move_iterator<InputIterator> >  move_range(InputIterator first, InputIterator last)
{
	return oetl::make_range(make_move_iter(first), make_move_iter(last));
}

} // namespace oetl
