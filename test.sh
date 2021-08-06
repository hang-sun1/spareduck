rm build/ -rf
mkdir build
cd build
cmake .. 
cmake --build . --target spareduck_test -j
./spareduck_test