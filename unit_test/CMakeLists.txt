cmake_minimum_required(VERSION 3.1)

project(oel-test CXX)

set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ standard to use, tested up to 20")


option(BUILD_GMOCK   "" off)
option(INSTALL_GTEST "" off)
option(gtest_force_shared_crt "" on)
add_subdirectory(googletest)


include(CheckCXXSourceCompiles)

check_cxx_source_compiles(
	"#if __has_include(<new>)
	int main() {}
	#endif"
	CAN_HAS_INCLUDE
)
if(NOT CAN_HAS_INCLUDE)
	add_definitions(-DOEL_NO_BOOST)
endif()

add_executable(oel-test
	dynarray_construct_assignop_swap_gtest.cpp
	dynarray_mutate_gtest.cpp
	dynarray_other_gtest.cpp
	forward_decl_test.cpp
	gtest_mem_main.cpp
	range_algo_gtest.cpp
	util_gtest.cpp
	view_gtest.cpp
	incl_dynarray.cpp
	incl_make_unique.cpp
	incl_range_algo.cpp
	incl_range_view.cpp
	incl_util.cpp
)

target_include_directories(oel-test PRIVATE
	${CMAKE_SOURCE_DIR}/..
)

if(MSVC)
	add_definitions(/D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)
	target_compile_options(oel-test PRIVATE /W4)
else()
	target_compile_options(oel-test PRIVATE -Wall -Wextra -Wshadow -Wconversion -Wno-sign-conversion)
endif()

target_link_libraries(oel-test gtest_main)
