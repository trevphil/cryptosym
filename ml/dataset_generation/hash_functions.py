# -*- coding: utf-8 -*-
import hashlib
from binascii import unhexlify
from BitVector import BitVector

from nsha256 import SHA256


def int2bytes(val):
  """ Converts a Python integer to a Python `bytes` object """

  # one (1) hex digit per four (4) bits
  width = val.bit_length()

  # unhexlify wants an even multiple of eight (8) bits, but we don't
  # want more digits than we need (hence the ternary-ish 'or')
  width += 8 - ((width % 8) or 8)

  # format width specifier: four (4) bits per hex digit
  fmt = '%%0%dx' % (width // 4)

  # unhexlify requires an even number of hex digits
  s = fmt % val
  if len(s) % 2 != 0:
    s = '0' + s

  return unhexlify(s)


def hashFunc(hash_input, hash_mode, difficulty):
  """
  Parameters:
   - hash_input: a BitVector input which will be hashed
   - hash_mode: control which hash function is used (real SHA-256 or something for testing)
   - difficulty: a parameter specifically for SHA-256, controlling how many rounds (normal=64)
  Returns:
   - A tuple where the first element is a 256-bit BitVector representing the hash of the input,
     and the second element is another BitVector for additional, optional random variables from
     the internals of the hash algorithm.
  """

  num_input_bits = hash_input.length()
  empty_bv = BitVector(size=0)

  if hash_mode == 'sha256':
    assert num_input_bits % 8 == 0, 'num_input_bits must be a multiple of 8'
    asbytes = (int(hash_input)).to_bytes(num_input_bits // 8, byteorder='big')
    return SHA256(asbytes, difficulty=difficulty).getData()

  elif hash_mode == 'md5':
    hash_val = hashlib.md5(int2bytes(int(hash_input))).hexdigest()
    return BitVector(intVal=int(hash_val, 16), size=256), empty_bv

  elif hash_mode == 'map_from_input':
    result = BitVector(size=0)
    for i in range(256):
      input_idx = int(i * num_input_bits / 256)
      result += BitVector(intVal=hash_input[input_idx], size=1)
    return result, empty_bv

  elif hash_mode == 'conditioned_on_input_and_hash':
    mapping = {
      0b00: BitVector(intVal=0b1, size=1),
      0b01: BitVector(intVal=0b0, size=1),
      0b10: BitVector(intVal=0b1, size=1),
      0b11: BitVector(intVal=0b0, size=1)
    }

    result = hash_input[:2]
    for i in range(2, 256):
      input_idx = int(i * num_input_bits / 256)
      result += mapping[int(result[i - 2] + hash_input[input_idx])]
    return result.reverse(), empty_bv

  elif hash_mode == 'pseudo_hash':
    num = int(hash_input)
    A, B, C, D = 0xAC32, 0xFFE1, 0xBF09, 0xBEEF
    a = ((num >> 0 ) & 0xFFFF) ^ A
    b = ((num >> 16) & 0xFFFF) ^ B
    c = ((num >> 32) & 0xFFFF) ^ C
    d = ((num >> 48) & 0xFFFF) ^ D
    a = (a | b)
    b = (b + c) & 0xFFFF
    c = (c ^ d)
    s = a | (b << 16) | (c << 32) | (d << 48)
    return BitVector(intVal=s, size=256), empty_bv

  else:
    raise 'Hash algorithm "{}" is not supported'.format(hash_mode)
