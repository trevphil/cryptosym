#!/bin/bash

echo "Warning: this script likely won't work on Windows"

set -e
set -x

sh -c './scripts/clean.sh'

Python_EXECUTABLE=$(python -c 'import sys; print(sys.executable)')
Python_INCLUDE_DIR=$(python -c 'import sysconfig; print(sysconfig.get_paths()["include"])')
Python_LIBRARY=$(python -c 'import sysconfig; print(sysconfig.get_paths()["stdlib"])')

mkdir build
pushd build
cmake -GNinja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPYBIND11_FINDPYTHON=ON \
  -DPython_EXECUTABLE="$Python_EXECUTABLE" \
  -DPython_INCLUDE_DIR="$Python_INCLUDE_DIR" \
  -DPython_LIBRARY="$Python_LIBRARY" \
  -DPYTHON_EXECUTABLE="$Python_EXECUTABLE" \
  -DPYTHON_INCLUDE_DIRS="$Python_INCLUDE_DIR" \
  -DPYTHON_LIBRARIES="$Python_LIBRARY" \
  ..
cmake --build .
popd

ctest --test-dir build -C RelWithDebInfo
