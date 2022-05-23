#!/bin/bash

echo "Warning: this script likely won't work on Windows"

set -e
set -x

sh -c './scripts/clean.sh'

python -m pip install --no-cache-dir .
python -m pip install pytest

TEST_DIR=$(pwd)/tests
cd $(mktemp -d) && python -m pytest "$TEST_DIR"
