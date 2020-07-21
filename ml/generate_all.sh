#!/bin/bash

set -e

python -m dataset_generation.generate --num-samples 10 --num-input-bits 64 --hash-algo invert

# python -m dataset_generation.generate --num-samples 10 --num-input-bits 64 --hash-algo pseudoHash

python -m dataset_generation.generate --num-samples 10 --num-input-bits 64 --hash-algo shiftLeft

python -m dataset_generation.generate --num-samples 10 --num-input-bits 64 --hash-algo shiftRight

python -m dataset_generation.generate --num-samples 10 --num-input-bits 64 --hash-algo xorConst
