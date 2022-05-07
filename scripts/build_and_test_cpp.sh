#!/bin/bash

set -e

git submodule update --init --recursive

if [ ! -d build ]; then
  mkdir build
fi

pushd build
cmake ..
make -j4
mv main ..

if [ -f unit_tests ]; then
  mv unit_tests ..
fi

popd
./unit_tests
