# Build script from https://medium.com/@tdeniffel/pragmatic-compiling-from-c-to-webassembly-a-guide-a496cc5954b8
rm build/ -rf
mkdir build
cd build
CC=emcc
CXX=em++
cmake -D CMAKE_C_COMPILER=emcc -D CMAKE_CXX_COMPILER=em++ .. 
make
mv spareduck.js ../www/
mkdir -p ../www/dist
cp spareduck.wasm ../www/dist
mv spareduck.wasm ../www