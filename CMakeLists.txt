cmake_minimum_required(VERSION 3.10)
project(spareduck VERSION 0.1.0)
file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true ./src/*.h ./src/*.cpp)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "--bind -O3 -s ENVIRONMENT=\"web\" -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME=\"spareduck\" -Wall -msimd128 -s LLD_REPORT_UNDEFINED -s ASSERTIONS=1 -msse2 -mssse3")
add_executable(spareduck.js ${SOURCES})

