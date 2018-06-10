#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"


namespace oel
{

template<typename T, typename Ctr>
struct is_trivially_relocatable< std::stack<T, Ctr> >
 :	is_trivially_relocatable<Ctr> {};

template<typename T, typename Ctr>
struct is_trivially_relocatable< std::queue<T, Ctr> >
 :	is_trivially_relocatable<Ctr> {};

template<typename T, typename Ctr, typename C>
struct is_trivially_relocatable< std::priority_queue<T, Ctr, C> >
 :	is_trivially_relocatable<Ctr> {};

template<typename T>
struct is_trivially_relocatable< std::deque<T> > : std::true_type {};

template<typename T>
struct is_trivially_relocatable< std::list<T> > : std::true_type {};

template<typename T>
struct is_trivially_relocatable< std::forward_list<T> > : std::true_type {};

template<typename T, typename C>
struct is_trivially_relocatable< std::set<T, C> > : std::true_type {};

template<typename T, typename C>
struct is_trivially_relocatable< std::multi_set<T, C> >
 :	std::true_type {};

template<typename K, typename T, typename C>
struct is_trivially_relocatable< std::map<K, T, C> >
 :	std::true_type {};

template<typename K, typename T, typename C>
struct is_trivially_relocatable< std::multi_map<K, T, C> >
 :	std::true_type {};

template<typename T, typename H, typename P>
struct is_trivially_relocatable< std::unordered_set<T, H, P> >
 :	std::true_type {};

template<typename T, typename H, typename P>
struct is_trivially_relocatable< std::unordered_multi_set<T, H, P> >
 :	std::true_type {};

template<typename K, typename T, typename H, typename P>
struct is_trivially_relocatable< std::unordered_map<K, T, H, P> >
 :	std::true_type {};

template<typename K, typename T, typename H, typename P>
struct is_trivially_relocatable< std::unordered_multi_map<K, T, H, P> >
 :	std::true_type {};

} // namespace oel
