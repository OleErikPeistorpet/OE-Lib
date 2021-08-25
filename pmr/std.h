#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../fwd.h"

#include <memory_resource>

namespace oel
{
//! Mirroring std::pmr and boost::container::pmr
namespace pmr
{
	using std::pmr::polymorphic_allocator;

	template< typename T >
	using dynarray = oel::dynarray< T, polymorphic_allocator<T> >;
}

}