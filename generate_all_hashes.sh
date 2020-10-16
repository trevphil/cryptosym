#!/bin/bash

set -e

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo lossyPseudoHash --visualize --difficulty 4

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo nonLossyPseudoHash --visualize --difficulty 1

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo invert --visualize

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo shiftLeft --visualize

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo shiftRight --visualize

python -m dataset_generation.generate --num-samples 4096 --num-input-bits 64 --hash-algo xorConst --visualize

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo orConst --visualize

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo andConst --visualize

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo addConst --visualize

python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo add --visualize

# SHA-256 algorithms
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 1
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 4
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 8
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 12
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 16
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 17
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 18
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 19
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 20
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 21
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 22
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 23
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 24
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 32
python -m dataset_generation.generate --num-samples 8 --num-input-bits 64 --hash-algo sha256 --difficulty 64
