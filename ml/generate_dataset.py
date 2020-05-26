# -*- coding: utf-8 -*-
import os
import sys
import random
import hashlib
from binascii import unhexlify

"""
This generates a CSV dataset of random hash inputs of length 64 bits
and their SHA-256 hashes (256 bits).

Each line in the generated file is a comma-separated list of the
256 hash bits followed by the 64 bits of the hash input.
"""

NUM_SAMPLES = int(sys.argv[1]) if len(sys.argv) > 1 else 20000
HASH_INPUT_NBITS = 64
INDEX_OF_BIT_TO_PREDICT = 256 + 0 # a.k.a. the first bit of the input message

HASH_MODES = [
  'sha256',
  'map_from_input', # each hash bit is equivalent to a bit in the input message
  'conditioned_on_input', # use simple CPDs conditioned on a single hash input bit
  'conditioned_on_input_and_hash' # use more complex CPDs conditioned on hash and input msg
]


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


def hashFunc(binstr, hash_mode=HASH_MODES[0]):
  """
  Parameters:
   - binstr: the input to the hash function (a string of bits, 0 or 1)
   - hash_mode: control which hash function is used (real SHA-256 or something for testing)
  Returns:
   - A 256-bit binary string of 0's and 1's representing the hash of the input
  """

  if hash_mode == HASH_MODES[0]:
    num = int2bytes(int(binstr, 2))
    hash_val = hashlib.sha256(num).hexdigest()
    binhash = bin(int(hash_val, 16))[2:]
    return ('0' * (256 - len(binhash))) + binhash
  elif hash_mode == HASH_MODES[1]:
    binhash = ''
    for i in range(256):
      input_idx = int(i * HASH_INPUT_NBITS / 256)
      binhash += binstr[input_idx]
    return binhash
  elif hash_mode == HASH_MODES[2]:
    binhash = ''
    for i in range(256):
      input_idx = int(i * HASH_INPUT_NBITS / 256)
      random_sample = random.random()
      if binstr[input_idx] == '1':
        binhash += '1' if random_sample <= 0.9 else '0'
      else:
        binhash += '0' if random_sample <= 0.85 else '1'
    return binhash
  elif hash_mode == HASH_MODES[3]:
    mapping = {'00': '1', '01': '0', '10': '1', '11': '0'}
    binhash = binstr[:2]
    for i in range(2, 256):
      input_idx = int(i * HASH_INPUT_NBITS / 256)
      binhash += mapping[binhash[i - 2] + binstr[input_idx]]
    return binhash[::-1] # reverse
  else:
    raise 'Hash mode {} is not supported'.format(hash_mode)


if __name__ == '__main__':
  random.seed(0)
  
  datafile = './data/data.csv'
  
  if os.path.exists(datafile):
      os.remove(datafile)

  with open(datafile, 'w') as f:
    for n in range(NUM_SAMPLES):
      binstr = randomBinaryString(HASH_INPUT_NBITS)
      ground_truth = binstr
      binhash = hashFunc(binstr, hash_mode=HASH_MODES[0])
      dataset_entry = ','.join([c for c in (binhash + ground_truth)])
      f.write(dataset_entry + '\n')
