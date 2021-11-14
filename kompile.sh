#!/bin/bash

set -e

if [ ! -d build ]; then
  mkdir build
fi

cd build

cmake -D CMAKE_C_COMPILER=gcc -D CMAKE_CXX_COMPILER=g++ ..

make

mv preimage ../main
mv dataset_generator ../
