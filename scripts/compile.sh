#!/bin/bash

set -e

if [ ! -d build ]; then
  mkdir build
fi

pushd build

cmake -D CMAKE_C_COMPILER=gcc -D CMAKE_CXX_COMPILER=g++ ..

make

mv main ..

mv cnf_gen ..

if [ -f tests ]; then
  mv tests ..
fi

popd
