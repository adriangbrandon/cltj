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
                    ${CMAKE_HOME_DIRECTORY}/lib/hybridBV/include
                    ${CMAKE_HOME_DIRECTORY}/include)

link_directories(~/lib)
add_subdirectory(lib/hybridBV)

#BENCH EXECUTABLES
add_executable(build-cltj src/bench/build-cltj.cpp)
target_link_libraries(build-cltj sdsl divsufsort divsufsort64)
set_target_properties(build-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-cltj-v2 src/bench/build-cltj-v2.cpp)
target_link_libraries(build-cltj-v2 sdsl divsufsort divsufsort64)
set_target_properties(build-cltj-v2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-cltj-dyn src/bench/build-cltj-dyn.cpp)
target_link_libraries(build-cltj-dyn sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-cltj-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-xcltj src/bench/build-xcltj.cpp)
target_link_libraries(build-xcltj sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-xcltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-xcltj-v2 src/bench/build-xcltj-v2.cpp)
target_link_libraries(build-xcltj-v2 sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-xcltj-v2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)


add_executable(build-xcltj-dyn src/bench/build-xcltj-dyn.cpp)
target_link_libraries(build-xcltj-dyn sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-xcltj-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-uncltj src/bench/build-uncltj.cpp)
target_link_libraries(build-uncltj sdsl divsufsort divsufsort64)
set_target_properties(build-uncltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-cltj-rdf src/bench/build-cltj-rdf.cpp)
target_link_libraries(build-cltj-rdf sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-cltj-rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(build-xcltj-rdf src/bench/build-xcltj-rdf.cpp)
target_link_libraries(build-xcltj-rdf sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(build-xcltj-rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-cltj src/bench/bench-query-cltj.cpp)
target_compile_definitions(bench-query-cltj PUBLIC ADAPTIVE=1)
target_link_libraries(bench-query-cltj sdsl divsufsort divsufsort64)
set_target_properties(bench-query-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-cltj-global src/bench/bench-query-cltj.cpp)
target_compile_definitions(bench-query-cltj-global PUBLIC ADAPTIVE=0)
target_link_libraries(bench-query-cltj-global sdsl divsufsort divsufsort64)
set_target_properties(bench-query-cltj-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-cltj-v2 src/bench/bench-query-cltj-v2.cpp)
target_compile_definitions(bench-query-cltj-v2 PUBLIC ADAPTIVE=1)
target_link_libraries(bench-query-cltj-v2 sdsl divsufsort divsufsort64)
set_target_properties(bench-query-cltj-v2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-cltj-v2-global src/bench/bench-query-cltj-v2.cpp)
target_compile_definitions(bench-query-cltj-v2-global PUBLIC ADAPTIVE=0)
target_link_libraries(bench-query-cltj-v2-global sdsl divsufsort divsufsort64)
set_target_properties(bench-query-cltj-v2-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-uncltj src/bench/bench-query-uncltj.cpp)
target_compile_definitions(bench-query-uncltj PUBLIC ADAPTIVE=1)
target_link_libraries(bench-query-uncltj sdsl divsufsort divsufsort64)
set_target_properties(bench-query-uncltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-uncltj-global src/bench/bench-query-uncltj.cpp)
target_compile_definitions(bench-query-uncltj-global PUBLIC ADAPTIVE=0)
target_link_libraries(bench-query-uncltj-global sdsl divsufsort divsufsort64)
set_target_properties(bench-query-uncltj-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-xcltj src/bench/bench-query-xcltj.cpp)
target_compile_definitions(bench-query-xcltj PUBLIC ADAPTIVE=1)
target_link_libraries(bench-query-xcltj sdsl divsufsort divsufsort64)
set_target_properties(bench-query-xcltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-xcltj-global src/bench/bench-query-xcltj.cpp)
target_compile_definitions(bench-query-xcltj-global PUBLIC ADAPTIVE=0)
target_link_libraries(bench-query-xcltj-global sdsl divsufsort divsufsort64)
set_target_properties(bench-query-xcltj-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-xcltj-v2 src/bench/bench-query-xcltj-v2.cpp)
target_compile_definitions(bench-query-xcltj-v2 PUBLIC ADAPTIVE=1)
target_link_libraries(bench-query-xcltj-v2 sdsl divsufsort divsufsort64)
set_target_properties(bench-query-xcltj-v2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-xcltj-v2-global src/bench/bench-query-xcltj-v2.cpp)
target_compile_definitions(bench-query-xcltj-v2-global PUBLIC ADAPTIVE=0)
target_link_libraries(bench-query-xcltj-v2-global sdsl divsufsort divsufsort64)
set_target_properties(bench-query-xcltj-v2-global PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-cltj-rdf src/bench/bench-query-cltj-rdf.cpp)
target_compile_definitions(bench-query-cltj-rdf PUBLIC ADAPTIVE=1)
target_link_libraries(bench-query-cltj-rdf sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-query-cltj-rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-query-xcltj-rdf src/bench/bench-query-xcltj-rdf.cpp)
target_compile_definitions(bench-query-xcltj-rdf PUBLIC ADAPTIVE=1)
target_link_libraries(bench-query-xcltj-rdf sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-query-xcltj-rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-update-cltj src/bench/bench-update-cltj.cpp)
target_compile_definitions(bench-update-cltj PUBLIC ADAPTIVE=1)
target_link_libraries(bench-update-cltj sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-update-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-update-xcltj src/bench/bench-update-xcltj.cpp)
target_compile_definitions(bench-update-xcltj PUBLIC ADAPTIVE=1)
target_link_libraries(bench-update-xcltj sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-update-xcltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-indels-cltj src/bench/bench-indels-cltj.cpp)
target_link_libraries(bench-indels-cltj sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-indels-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(bench-indels-xcltj src/bench/bench-indels-xcltj.cpp)
target_link_libraries(bench-indels-xcltj sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(bench-indels-xcltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

add_executable(stats-query-cltj src/bench/stats-query-cltj.cpp)
target_link_libraries(stats-query-cltj sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(stats-query-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench)

#EXAMPLE
add_executable(ex_dyn_rdf src/example/ex_dyn_rdf.cpp)
target_link_libraries(ex_dyn_rdf sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(ex_dyn_rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example)

add_executable(ex_dyn_ids src/example/ex_dyn_ids.cpp)
target_link_libraries(ex_dyn_ids sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(ex_dyn_ids PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example)

add_executable(ex_static_ids src/example/ex_static_ids.cpp)
target_link_libraries(ex_static_ids sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(ex_static_ids PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example)

#CMD
add_executable(cmd-cltj src/cmd/cmd-cltj.cpp)
target_link_libraries(cmd-cltj sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(cmd-cltj PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/cmd)

add_executable(cmd-cltj-rdf src/cmd/cmd-cltj-rdf.cpp)
target_link_libraries(cmd-cltj-rdf sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(cmd-cltj-rdf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/cmd)

#TEST
add_executable(test-index-dyn src/test/test-index-dyn.cpp)
target_link_libraries(test-index-dyn sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(test-index-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-index-static src/test/test-index-static.cpp)
target_link_libraries(test-index-static sdsl divsufsort divsufsort64)
set_target_properties(test-index-static PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(main src/test/main.cpp)
target_link_libraries(main sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)



add_executable(dyn-exp1 src/test/dyn-exp1.cpp)
target_compile_definitions(dyn-exp1 PUBLIC ADAPTIVE=1)
target_link_libraries(dyn-exp1 sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp1-metatrie src/test/dyn-exp1-metatrie.cpp)
target_compile_definitions(dyn-exp1-metatrie PUBLIC ADAPTIVE=1)
target_link_libraries(dyn-exp1-metatrie sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp1-metatrie PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp1-fixed src/test/dyn-exp1.cpp)
target_compile_definitions(dyn-exp1-fixed PUBLIC ADAPTIVE=0)
target_link_libraries(dyn-exp1-fixed sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp1-fixed PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp2 src/test/dyn-exp2.cpp)
target_compile_definitions(dyn-exp2 PUBLIC ADAPTIVE=1)
target_link_libraries(dyn-exp2 sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp2-fixed src/test/dyn-exp2.cpp)
target_compile_definitions(dyn-exp2-fixed PUBLIC ADAPTIVE=0)
target_link_libraries(dyn-exp2-fixed sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp2-fixed PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp3 src/test/dyn-exp3.cpp)
target_compile_definitions(dyn-exp3 PUBLIC ADAPTIVE=1)
target_link_libraries(dyn-exp3 sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(dyn-exp3-fixed src/test/dyn-exp3.cpp)
target_compile_definitions(dyn-exp3-fixed PUBLIC ADAPTIVE=0)
target_link_libraries(dyn-exp3-fixed sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(dyn-exp3-fixed PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-dict src/test/test-dict.cpp)
target_link_libraries(test-dict sdsl divsufsort divsufsort64)
set_target_properties(test-dict PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-regex src/test/test-regex.cpp)
target_link_libraries(test-regex sdsl divsufsort divsufsort64)
set_target_properties(test-regex PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-bulk-load src/test/test-bulk-load.cpp)
target_link_libraries(test-bulk-load sdsl divsufsort divsufsort64)
set_target_properties(test-bulk-load PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-mt-dyn src/test/test-mt-dyn.cpp)
target_link_libraries(test-mt-dyn sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(test-mt-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(interactive-metatrie src/test/interactive-metatrie.cpp)
target_link_libraries(interactive-metatrie sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(interactive-metatrie PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(update-index-triples src/test/update-index-triples.cpp)
target_link_libraries(update-index-triples sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(update-index-triples PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(prepare-80 src/test/prepare-80.cpp)
target_link_libraries(prepare-80 sdsl divsufsort divsufsort64)
set_target_properties(prepare-80 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(prepare-20 src/test/prepare-20.cpp)
target_link_libraries(prepare-20 sdsl divsufsort divsufsort64)
set_target_properties(prepare-20 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(test-dyn src/test/test-dyn.cpp)
target_link_libraries(test-dyn sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(test-dyn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)

add_executable(cltj-louds src/test/cltj_louds.cpp)
target_link_libraries(cltj-louds sdsl divsufsort divsufsort64 hybridbv_gn)
set_target_properties(cltj-louds PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)
