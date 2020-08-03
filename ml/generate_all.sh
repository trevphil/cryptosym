#!/bin/bash

set -e

# python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 1

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo invert

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo lossyPseudoHash

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo nonLossyPseudoHash

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo shiftLeft

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo shiftRight

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo xorConst

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo orConst

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo andConst

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo addConst

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo add
