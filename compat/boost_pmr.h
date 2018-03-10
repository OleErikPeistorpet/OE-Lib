#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "default.h"

#include <boost/container/pmr/polymorphic_allocator.hpp>


namespace oel
{
#ifndef OEL_HAS_STD_PMR
	//! Mirroring std::pmr and boost::container::pmr
	namespace pmr
	{
	using boost::container::pmr::polymorphic_allocator;

	template<typename T>
	using dynarray = oel::dynarray< T, polymorphic_allocator<T> >;
	}
#endif

template<typename T>
struct is_trivially_relocatable< boost::container::pmr::polymorphic_allocator<T> > : true_type {};

} // namespace oel
