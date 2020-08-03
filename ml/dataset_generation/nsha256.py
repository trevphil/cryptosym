# -*- coding: utf-8 -*-
#!/usr/bin/python3

import copy
import struct
import binascii
import hashlib
from BitVector import BitVector

from dataset_generation.sym_bit_vec import SymBitVec


F32 = SymBitVec(BitVector(intVal=0xFFFFFFFF, size=32))

_k = [0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
      0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
      0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
      0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
      0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
      0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
      0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
      0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
      0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
      0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
      0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
      0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
      0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2]
_k = [SymBitVec(BitVector(intVal=item, size=32)) for item in _k]

_h = [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19]
_h = [SymBitVec(BitVector(intVal=item, size=32)) for item in _h]


def pad(msglen):
  assert type(msglen) is int
  length = (msglen << 3) & 0xFFFFFFFFFFFFFFFF
  length = BitVector(intVal=length, size=64)

  mdi = msglen & 0x3F
  if mdi < 56:
    padlen = 55 - mdi
  else:
    padlen = 119 - mdi

  result = BitVector(size=padlen * 8)
  result = result + length
  result = BitVector(intVal=0x80, size=8) + result
  return SymBitVec(result)


def _rotr(x, y):
  return ((x >> y) | (x << (32 - y))) & F32


def _maj(x, y, z):
  return (x & y) ^ (x & z) ^ (y & z)


def _ch(x, y, z):
  return (x & y) ^ ((~x) & z)


class SHA256:

  def __init__(self, input_sym_bit_vec, difficulty=64):
    self._difficulty = difficulty
    self._counter = 0
    self._cache = SymBitVec(BitVector(size=0))
    self._k = copy.deepcopy(_k)
    self._h = copy.deepcopy(_h)

    int_val = int(input_sym_bit_vec)
    input_bytes = int_val.to_bytes((int_val.bit_length() + 7) // 8, 'big')
    self._expected_digest = hashlib.sha256(input_bytes).hexdigest()

    self._update(input_sym_bit_vec)


  def _compress(self, c):
    w = [SymBitVec(BitVector(size=32)) for _ in range(64)]
    for i in range(16):
      lower = i * 32
      upper = (i + 1) * 32
      w[i] = c.extract(lower, upper)

    for i in range(16, 64):
      s0 = _rotr(w[i-15], 7) ^ _rotr(w[i-15], 18) ^ (w[i-15] >> 3)
      s1 = _rotr(w[i-2], 17) ^ _rotr(w[i-2], 19) ^ (w[i-2] >> 10)
      w[i] = (w[i-16] + s0 + w[i-7] + s1) & F32

    a, b, c, d, e, f, g, h = self._h

    for i in range(self._difficulty):
      s0 = _rotr(a, 2) ^ _rotr(a, 13) ^ _rotr(a, 22)
      t2 = s0 + _maj(a, b, c)
      s1 = _rotr(e, 6) ^ _rotr(e, 11) ^ _rotr(e, 25)
      t1 = h + s1 + _ch(e, f, g) + self._k[i] + w[i]

      h = g
      g = f
      f = e
      e = (d + t1) & F32
      d = c
      c = b
      b = a
      a = (t1 + t2) & F32

    for i, (x, y) in enumerate(zip(self._h, [a, b, c, d, e, f, g, h])):
      self._h[i] = (x + y) & F32


  def _update(self, m):
    self._counter += (len(m) // 8)  # Add number of bytes in the message
    m = self._cache.concat(m)
    n = len(m)

    for i in range(0, n // 512):
      lower = i * 512
      upper = (i + 1) * 512
      self._compress(m.extract(lower, upper))

    self._cache = self._cache.extract(0, n % 512)


  def digest(self):
    self._update(pad(self._counter))
    
    result = self._h[0]
    for item in self._h[1:]:
      result = result.concat(item)
    
    if self._difficulty == 64:
      # Running the "true" SHA-256 algo, so make sure result is correct
      hex_digest = result.hex()
      hex_digest = ('0' * (64 - len(hex_digest))) + hex_digest
      if hex_digest != self._expected_digest:
        raise RuntimeError('Expected {}, got {}'.format(
          self._expected_digest, hex_digest))
      else:
        print('Hashes match: {}'.format(hex_digest))

    return result
