rm -rf build/ && mkdir build
cd /usr/src/spareduck/build
cmake -D CMAKE_C_COMPILER=emcc -D CMAKE_CXX_COMPILER=em++ .. && make -j8