#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../user_traits.h"

#include <memory>
#include <string>
#ifndef OEL_NO_BOOST
	#include <boost/optional/optional_fwd.hpp>
	#include <boost/circular_buffer_fwd.hpp>

	namespace boost {
	template<typename T> class intrusive_ptr;
	}
#endif

// std:: unique_ptr, shared_ptr, weak_ptr, basic_string
// boost:: optional, intrusive_ptr, circular_buffer

// This file is included by dynarray.h, so should not be needed in user code

namespace oel
{

/// std::unique_ptr assumed trivially relocatable if the deleter is
template<typename T, typename Del>
is_trivially_relocatable<Del> specify_trivial_relocate(std::unique_ptr<T, Del>);

template<typename T>
true_type specify_trivial_relocate(std::shared_ptr<T>);

template<typename T>
true_type specify_trivial_relocate(std::weak_ptr<T>);

// std::string in GCC 5 with _GLIBCXX_USE_CXX11_ABI is not trivially relocatable (uses pointer to internal buffer)
#if _MSC_VER || (__GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI) || _LIBCPP_VERSION
	template<typename C, typename Tr>
	true_type specify_trivial_relocate(std::basic_string<C, Tr>);
#endif

#ifndef OEL_NO_BOOST
	template<typename T>
	is_trivially_relocatable<T> specify_trivial_relocate(boost::optional<T> &&);

	template<typename T>
	true_type specify_trivial_relocate(boost::intrusive_ptr<T>);

	template<typename T>
	true_type specify_trivial_relocate(boost::circular_buffer<T>);
#endif

}