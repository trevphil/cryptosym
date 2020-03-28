# -*- coding: utf-8 -*-
import random
import hashlib
from binascii import unhexlify

"""
This generates a CSV dataset of random hash inputs of length 64 bits
and their SHA-256 hashes (256 bits).

Each line in the generated file is a comma-separated list of the
256 hash bits followed by the 64 bits of the hash input.
"""

NUM_SAMPLES = 20000
HASH_INPUT_NBITS = 64
INDEX_OF_BIT_TO_PREDICT = 0


def int2bytes(val):
  # one (1) hex digit per four (4) bits
  width = val.bit_length()

  # unhexlify wants an even multiple of eight (8) bits, but we don't
  # want more digits than we need (hence the ternary-ish 'or')
  width += 8 - ((width % 8) or 8)

  # format width specifier: four (4) bits per hex digit
  fmt = '%%0%dx' % (width // 4)

  # prepend zero (0) to the width, to zero-pad the output
  s = unhexlify(fmt % val)
  return s


def randomBinaryString(nbits):
  x = random.randint(0, int(2**nbits))
  binstr = bin(x)[2:]
  num_zero_padding = nbits - len(binstr)
  return ('0' * num_zero_padding) + binstr


with open('./data/data.csv', 'w') as f:
  for n in range(NUM_SAMPLES):
    binstr = randomBinaryString(HASH_INPUT_NBITS)
    ground_truth = binstr
    num = int2bytes(int(binstr, 2))

    hash = hashlib.sha256(num).hexdigest()
    binhash = bin(int(hash, 16))[2:]
    binhash = ('0' * (256 - len(binhash))) + binhash

    dataset_entry = ','.join([c for c in (binhash + ground_truth)])
    f.write(dataset_entry + '\n')
