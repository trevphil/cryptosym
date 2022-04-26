#!/bin/bash

set -e

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
