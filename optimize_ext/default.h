#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// std:: unique_ptr, shared_ptr, weak_ptr, basic_string, pair, tuple
// boost:: intrusive_ptr, local_shared_ptr, circular_buffer, variant, polymorphic_allocator

/** @file
* @brief This is included by dynarray.h, so should not be needed in user code
*/

#include "../auxi/core_util.h"

#include <memory>
#include <tuple>

#if __has_include(<boost/config.hpp>)
	#define OEL_HAS_BOOST  1
#endif

#ifdef OEL_HAS_BOOST

#include <boost/circular_buffer_fwd.hpp>
#include <boost/container/container_fwd.hpp>
#include <boost/variant/variant_fwd.hpp>

namespace boost
{
	template< typename T > class intrusive_ptr;
	template< typename T > class local_shared_ptr;
}
#endif


// std::string in GNU library with _GLIBCXX_USE_CXX11_ABI is not trivially relocatable (uses pointer to internal buffer)
#if (defined _CPPLIB_VER or defined _LIBCPP_VERSION or defined __GLIBCXX__) and !_GLIBCXX_USE_CXX11_ABI

#include <string>

namespace oel
{
	template< typename C, typename Tr, typename Alloc >
	struct is_trivially_relocatable< std::basic_string<C, Tr, Alloc> >
	 :	bool_constant
		<	is_trivially_relocatable<Alloc>::value and
			is_trivially_relocatable< typename std::allocator_traits<Alloc>::pointer >::value
		> {};
}
#endif

namespace oel
{

template< typename T >
struct is_trivially_relocatable< std::allocator<T> > : true_type {};

template< typename T, typename Del >
struct is_trivially_relocatable< std::unique_ptr<T, Del> >
 :	is_trivially_relocatable<Del> {};

template< typename T >
struct is_trivially_relocatable< std::shared_ptr<T> > : true_type {};

template< typename T >
struct is_trivially_relocatable< std::weak_ptr<T> > : true_type {};

#ifdef OEL_HAS_BOOST
	template< typename T >
	struct is_trivially_relocatable< boost::container::pmr::polymorphic_allocator<T> > : true_type {};

	template< typename T >
	struct is_trivially_relocatable< boost::intrusive_ptr<T> > : true_type {};

	template< typename T >
	struct is_trivially_relocatable< boost::local_shared_ptr<T> > : true_type {};

	template< typename T, typename Alloc >
	struct is_trivially_relocatable< boost::circular_buffer<T, Alloc> >
	 :	bool_constant
		<	is_trivially_relocatable<Alloc>::value and
			is_trivially_relocatable< typename std::allocator_traits<Alloc>::pointer >::value
		> {};

	template< typename... Ts >
	struct is_trivially_relocatable< boost::variant<Ts...> >
	 :	std::conjunction< is_trivially_relocatable<Ts>... > {};
#endif

template< typename T, typename U >
struct is_trivially_relocatable< std::pair<T, U> >
 :	std::conjunction< is_trivially_relocatable<T>, is_trivially_relocatable<U> > {};

template< typename... Ts >
struct is_trivially_relocatable< std::tuple<Ts...> >
 :	std::conjunction< is_trivially_relocatable<Ts>... > {};

}