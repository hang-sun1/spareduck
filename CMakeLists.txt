cmake_minimum_required(VERSION 3.10)
project(spareduck VERSION 0.1.0)
set(CMAKE_CXX_FLAGS "-O2 -s ENVIRONMENT=\"web\" -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME=\"spareduck\"")
add_executable(spareduck.js src/main.cpp)

