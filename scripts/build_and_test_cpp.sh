#!/bin/bash

set -e

if [ ! -d build ]; then
  mkdir build
fi

pushd build
cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build .
popd

ctest --test-dir build -C RelWithDebInfo
