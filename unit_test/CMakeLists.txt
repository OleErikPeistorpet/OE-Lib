cmake_minimum_required(VERSION 3.1)

project(oel-test CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to use, minimum 17")

option(MEM_BOUND_DEBUG "Set OEL_MEM_BOUND_DEBUG_LVL=2" off)


option(BUILD_GMOCK   "" off)
option(INSTALL_GTEST "" off)
option(gtest_force_shared_crt "" on)
add_subdirectory(googletest)


add_executable(oel-test
	dynarray_construct_assignop_swap_gtest.cpp
	dynarray_mutate_gtest.cpp
	dynarray_other_gtest.cpp
	forward_decl_test.cpp
	gtest_mem_main.cpp
	range_algo_gtest.cpp
	util_gtest.cpp
	view_gtest.cpp
	incl_allocator.cpp
	incl_dynarray.cpp
	incl_range_algo.cpp
	incl_util.cpp
	incl_view_counted.cpp
	incl_view_move.cpp
	incl_view_subrange.cpp
	incl_view_transform.cpp
	incl_transform_iterator.cpp
)

target_include_directories(oel-test PRIVATE
	${CMAKE_SOURCE_DIR}/..
)

if(MEM_BOUND_DEBUG)
	add_definitions(-D OEL_MEM_BOUND_DEBUG_LVL=2)
endif()
if(MSVC)
	add_definitions(/D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)
	target_compile_options(oel-test PRIVATE /W4)
else()
	if(MEM_BOUND_DEBUG)
		target_compile_options(oel-test PRIVATE -fno-strict-aliasing)
	endif()
	target_compile_options(oel-test PRIVATE -Wall -Wextra -Wold-style-cast -Wshadow -Wconversion -Wno-sign-conversion)
endif()

target_link_libraries(oel-test gtest_main)
