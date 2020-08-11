#!/bin/bash

set -e

python -m dataset_generation.generate --num-samples 16 --num-input-bits 512 --hash-algo sha256 --difficulty 64

python -m dataset_generation.generate --num-samples 16 --num-input-bits 64 --hash-algo lossyPseudoHash --difficulty 4

python -m dataset_generation.generate --num-samples 16 --num-input-bits 64 --hash-algo nonLossyPseudoHash --difficulty 4

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo invert

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo shiftLeft

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo shiftRight

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo xorConst

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo orConst

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo andConst

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo addConst

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo add
