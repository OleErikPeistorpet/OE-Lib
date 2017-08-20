#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "user_traits.h"

#if defined(__has_include)
#if __has_include(<memory_resource>)
	#include <memory_resource>

	#define OEL_STD_MEMORY_RESOURCE
#endif
#endif

#ifndef OEL_STD_MEMORY_RESOURCE
	#include <boost/container/pmr/polymorphic_allocator.hpp>
#endif

namespace oel
{
namespace pmr
{
#ifdef OEL_STD_MEMORY_RESOURCE
	using std::pmr::polymorphic_allocator;
#else
	using boost::container::pmr::polymorphic_allocator;
#endif

template<typename T>
using dynarray = oel::dynarray< T, polymorphic_allocator<T> >;
}

template<typename T>
struct is_trivially_relocatable< pmr::polymorphic_allocator<T> > : true_type {};

} // namespace oel
