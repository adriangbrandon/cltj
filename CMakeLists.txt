cmake_minimum_required(VERSION 2.8.7)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(AppendCompilerFlags)
set(CMAKE_OSX_ARCHITECTURES x86_64)

project(CLTJ)
set(CMAKE_PROJECT_NAME "CLTJ")

#Change to "Debug" when you need it
set(CMAKE_BUILD_TYPE "Release")

# C++11 compiler Check
if(NOT CMAKE_CXX_COMPILER_VERSION) # work around for cmake versions smaller than 2.8.10
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION)
endif()
if(CMAKE_CXX_COMPILER MATCHES ".*clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()
if( (CMAKE_COMPILER_IS_GNUCXX AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.7) OR
(CMAKE_COMPILER_IS_CLANGXX AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 3.2))
    message(FATAL_ERROR "Your C++ compiler does not support C++11. Please install g++ 4.7 (or greater) or clang 3.2 (or greater)")
else()
    message(STATUS "Compiler is recent enough to support C++11.")
endif()
if( CMAKE_COMPILER_IS_GNUCXX )
    append_cxx_compiler_flags("-fpermissive -std=c++11 -Wall -Wextra " "GCC" CMAKE_CXX_FLAGS)
    append_cxx_compiler_flags("-msse4.2 -O3 -ffast-math -funroll-loops -fno-omit-frame-pointer -g" "GCC" CMAKE_CXX_FLAGS_RELEASE)
else()
    append_cxx_compiler_flags("-fpermissive -std=c++11" "CLANG" CMAKE_CXX_FLAGS)
    append_cxx_compiler_flags("-stdlib=libc++" "CLANG" CMAKE_CXX_FLAGS)
    append_cxx_compiler_flags("-msse4.2 -O3  -ffast-math -funroll-loops -DNDEBUG" "CLANG" CMAKE_CXX_FLAGS_RELEASE)
endif()
include(CheckSSE)
FindSSE ()
if( SSE4_2_FOUND )
    if( CMAKE_COMPILER_IS_GNUCXX )
        append_cxx_compiler_flags("-msse4.2" "GCC" CMAKE_CXX_FLAGS)
    else()
        append_cxx_compiler_flags("-msse4.2" "CLANG" CMAKE_CXX_FLAGS)
    endif()
    message(STATUS "CPU does support SSE4.2.")
else()
    message(STATUS "CPU does NOT support SSE4.2")
endif()

include_directories(~/include
                    ${CMAKE_HOME_DIRECTORY}/include)

link_directories(~/lib)


add_executable(build-index src/build-index.cpp)
target_link_libraries(build-index sdsl divsufsort divsufsort64)

add_executable(build-uncltj-index src/build-uncltj-index.cpp)
target_link_libraries(build-uncltj-index sdsl divsufsort divsufsort64)

add_executable(query-index src/query-index.cpp)
target_compile_definitions(query-index PUBLIC ADAPTIVE=1)
target_link_libraries(query-index sdsl divsufsort divsufsort64)

add_executable(query-index-fixed src/query-index.cpp)
target_compile_definitions(query-index-fixed PUBLIC ADAPTIVE=0)
target_link_libraries(query-index-fixed sdsl divsufsort divsufsort64)


add_executable(query-uncltj-index src/query-uncltj-index.cpp)
target_compile_definitions(query-uncltj-index PUBLIC ADAPTIVE=1)
target_link_libraries(query-uncltj-index sdsl divsufsort divsufsort64)

add_executable(query-uncltj-index-fixed src/query-uncltj-index.cpp)
target_compile_definitions(query-uncltj-index-fixed PUBLIC ADAPTIVE=0)
target_link_libraries(query-uncltj-index-fixed sdsl divsufsort divsufsort64)
