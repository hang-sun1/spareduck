rm build/ -rf
mkdir build
cd build
cmake ../test
make
./spareduck_test