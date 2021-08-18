cmake_minimum_required(VERSION 3.10)
project(spareduck VERSION 0.1.0)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true ./src/*.hpp ./src/*.cpp)
file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES true ./test/*.hpp ./test/*.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O3 -Wall -msse -msse2 -mssse3 -msse4.1 -g")

add_library(spareduck_lib STATIC ${SOURCES})
add_executable(spareduck ${SOURCES})
add_executable(spareduck_test ${TEST_SOURCES})

set_target_properties(spareduck PROPERTIES LINK_FLAGS "--bind -s ENVIRONMENT=web -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME=\"spareduck\" -s LLD_REPORT_UNDEFINED -s ASSERTIONS=1 -s MAXIMUM_MEMORY=4GB -s INITIAL_MEMORY=1GB -s FETCH=1")
set_target_properties(spareduck PROPERTIES COMPILE_FLAGS "-msimd128")

set_target_properties(spareduck_test PROPERTIES COMPILE_FLAGS "-DTESTING")
target_link_libraries(spareduck_test PUBLIC spareduck_lib)

set_target_properties(spareduck_lib PROPERTIES COMPILE_FLAGS "-DTESTING")

