#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../type_traits.h"

#include <stdexcept>

namespace oel
{
namespace _detail
{
	struct Throw
	{	// Exception throwing has been split out from templates to avoid bloat
		[[noreturn]] static void outOfRange(const char * what)
		{
			OEL_THROW(std::out_of_range(what), what);
		}

		[[noreturn]] static void lengthError(const char * what)
		{
			OEL_THROW(std::length_error(what), what);
		}
	};
}

}