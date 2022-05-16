#!/bin/bash

set -e

if [ ! -d build ]; then
  mkdir build
fi

pushd build
cmake -GNinja .. && cmake --build .
popd

ctest --test-dir build
