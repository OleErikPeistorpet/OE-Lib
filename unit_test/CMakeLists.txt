cmake_minimum_required(VERSION 3.1)

project(oel-test CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)


# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
)
if(result)
	message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
)
if(result)
	message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to build. This defines the gtest and gtest_main targets
add_subdirectory(
	${CMAKE_CURRENT_BINARY_DIR}/googletest-src
	${CMAKE_CURRENT_BINARY_DIR}/googletest-build
	EXCLUDE_FROM_ALL
)


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
	inplace_dynarr_gtest.cpp
	range_algo_gtest.cpp
	util_gtest.cpp
	incl_dynarray.cpp
	incl_inplace_dynarr.cpp
	incl_make_unique.cpp
	incl_range_algo.cpp
	incl_range_view.cpp
	incl_util.cpp
)

target_include_directories(oel-test PRIVATE
	${CMAKE_SOURCE_DIR}/..
)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(oel-test PRIVATE -Wall -Wextra)
endif()

target_link_libraries(oel-test gtest_main)
