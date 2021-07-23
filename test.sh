rm build/ -rf
mkdir build
cd build
cmake ../test
make -j8
./spareduck_test