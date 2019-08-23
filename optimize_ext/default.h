#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// std:: unique_ptr, shared_ptr, weak_ptr, basic_string, pair, tuple
// boost:: intrusive_ptr, circular_buffer, variant, polymorphic_allocator

/** @file
* @brief This is included by dynarray.h, so should not be needed in user code
*/

#include "../auxi/type_traits.h"

#include <memory>
#include <tuple>

#ifndef OEL_NO_BOOST

#include <boost/circular_buffer_fwd.hpp>
#include <boost/container/container_fwd.hpp>
#include <boost/variant/variant_fwd.hpp>

namespace boost
{
	template<typename T> class intrusive_ptr;
}
#endif


// std::string in GNU library with _GLIBCXX_USE_CXX11_ABI is not trivially relocatable (uses pointer to internal buffer)
#if (defined _CPPLIB_VER or defined _LIBCPP_VERSION or defined __GLIBCXX__) and !_GLIBCXX_USE_CXX11_ABI

#include <string>

namespace oel
{
	//! Assumed trivially relocatable if the allocator is. Could also check the pointer type for extra assurance
	template<typename C, typename Tr, typename Alloc>
	struct is_trivially_relocatable< std::basic_string<C, Tr, Alloc> >
	 :	is_trivially_relocatable<Alloc> {};
}
#endif

namespace oel
{

template<typename T, typename Del>
struct is_trivially_relocatable< std::unique_ptr<T, Del> >
 :	is_trivially_relocatable<Del> {};

template<typename T>
struct is_trivially_relocatable< std::shared_ptr<T> > : true_type {};

template<typename T>
struct is_trivially_relocatable< std::weak_ptr<T> > : true_type {};

#ifndef OEL_NO_BOOST
	template<typename T>
	struct is_trivially_relocatable< boost::container::pmr::polymorphic_allocator<T> > : true_type {};

	template<typename T>
	struct is_trivially_relocatable< boost::intrusive_ptr<T> > : true_type {};

	template<typename T, typename Alloc>
	struct is_trivially_relocatable< boost::circular_buffer<T, Alloc> >
	 :	is_trivially_relocatable<Alloc> {};

	template<typename... Ts>
	struct is_trivially_relocatable< boost::variant<Ts...> >
	 :	all_< is_trivially_relocatable<Ts>... > {};
#endif


template<typename T, typename U>
struct is_trivially_relocatable< std::pair<T, U> >
 :	all_< is_trivially_relocatable<T>, is_trivially_relocatable<U> > {};

template<typename... Ts>
struct is_trivially_relocatable< std::tuple<Ts...> >
 :	all_< is_trivially_relocatable<Ts>... > {};

}