#!/bin/bash

echo "Warning: this script likely won't work on Windows"

pip install --upgrade pip
pip uninstall -y cryptosym
rm -rf build
rm -rf "*.egg-info"

pip install --no-cache-dir . && \
  pytest tests
