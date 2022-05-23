#!/bin/bash

python -m pip uninstall -y cryptosym

rm -rf ./build
rm -rf ./wheelhouse
rm -rf ./cryptosym/*.so
rm -rf .pytest_cache
find . -type d -name "*.egg-info" -prune -exec rm -rf {} \;
find . -type d -name __pycache__ -prune -exec rm -rf {} \;
