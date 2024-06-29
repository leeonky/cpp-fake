#!/bin/bash

# Build cpputest
echo "Building cpputest..."
cd cpputest
./configure --disable-longlong
make clean && make -j 4 tdd
cd ..

# Build and run the test
echo "Building and running the test..."
make clean && make all

# Build and run the program
#echo "Building and running the program..."
#make main