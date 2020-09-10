# -*- coding: utf-8 -*-
import codecs
import copy
from time import time
from z3 import *

_NONCE_LEN = 32 # bits
_BLOCK_HEADER_LEN = 640 # bits
_SHA256_HASH_LEN = 256 # bits

_F32 = 0xFFFFFFFF

_K = [0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
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

_H = [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19]


class NSHA256Reverse:
  """Reverse-engineer SHA-256 hash function"""


  def __init__(self,
    block_header_no_nonce=None,  # Block header (as an integer) without the nonce
    block_hash=None):  # The double SHA-256 hash of the block (hex string or int)

    self.__counter = 0
    self.__cache = None
    self.__k = copy.deepcopy(_K)
    self.__h = [BitVecVal(h, 32) for h in _H]

    if block_header_no_nonce is None or block_hash is None:
      return

    assert type(block_header_no_nonce) is int, 'Wrong type given for block_header_no_nonce'

    self.__solver = Solver()
    self.__block_hash = int(block_hash, 16) if type(block_hash) is str else block_hash
    self.__nonce = BitVec('nonce', _NONCE_LEN)
    prefix = BitVecVal(block_header_no_nonce, _BLOCK_HEADER_LEN - _NONCE_LEN)
    # The block header guess is the input to the double SHA-256 hash.
    # It is like a variable, where the first part of the variable
    # is known and the last part (the nonce) is unknown. We want to solve the nonce.
    self.__block_header_guess = Concat(prefix, self.__nonce)
    # We don't know what the hash of the block header will be, only that it
    # needs to be below a certain value to be accepted by BitCoin (in other
    # words, it needs a certain number of 0's in the least-significant bits).
    self.__hash_guess = BitVec('hash_guess', _SHA256_HASH_LEN)


  ## Public functions


  def reverse_hash256(self):
    start_time = time()

    # Feed the block header guess (an unknown variable) into SHA-256, twice
    algo = self
    algo.__update(self.__block_header_guess)

    # First pass through SHA-256
    n = _SHA256_HASH_LEN
    first_digest = algo.digest()
    assert first_digest.size() == n, 'Wrong number of digest bits for a SHA-256 hash'

    # Second pass through SHA-256
    algo = NSHA256Reverse()
    algo.__update(first_digest)
    second_digest = algo.digest()
    assert second_digest.size() == n, 'Wrong number of digest bits for a SHA-256 hash'

    # Important: put a constraint that our hash variable must equal the resulting
    # hash when the block header is fed through SHA-256 twice
    self.__solver.add(self.__hash_guess == second_digest)

    # Now we constrain the number of zeros that must be in the resulting hash,
    # which should technically also enforce constraints on the nonce in the
    # block header, since we have created a dependency between the hash guess
    # and the second digest of the block header.
    # So when the solver does its magic, we get the right nonce.
    required_zeros = 64
    hash_ending = Extract(required_zeros - 1, 0, self.__hash_guess)
    self.__solver.add(hash_ending == 0)

    # Check if the problem has a solution
    status = self.__solver.check()
    print('\nSatisfiability = {}'.format(status))
    end_time = time()

    print('Run time: %f sec' % round(end_time - start_time, 3))

    if status == sat:
      model = self.__solver.model()

      # If there's a solution, print the nonce and the resulting hash
      nonce = model[self.__nonce].as_long()
      nonce = hex(nonce)[2:].replace('L', '')

      hash_guess = model[self.__hash_guess].as_long()
      hash_guess = hex(hash_guess)[2:]
      sha256_hex_len = _SHA256_HASH_LEN // 4
      hash_guess = '0' * (sha256_hex_len - len(hash_guess)) + hash_guess

      return (True, nonce, hash_guess)

    return (False, None, None)


  def digest(self):
    self.__update(self.__pad(self.__counter))

    result = self.__h[0]
    for bv in self.__h[1:]:
      result = Concat(result, bv)

    return result


  ## Private functions


  def __rotr(self, x, y):
    return (LShR(x, y) | (x << (32 - y))) & _F32


  def __maj(self, x, y, z):
    return (x & y) ^ (x & z) ^ (y & z)


  def __ch(self, x, y, z):
    return (x & y) ^ ((~x) & z)


  def __pad(self, msglen):
    length = (msglen << 3) & 0xffffffffffffffff
    length = BitVecVal(length, 64)

    mdi = msglen & 0x3F
    if mdi < 56:
      padlen = 55 - mdi
    else:
      padlen = 119 - mdi

    result = BitVecVal(0, padlen * 8)
    result = Concat(result, length)
    result = Concat(BitVecVal(0x80, 8), result)
    return result


  def __compress(self, c):
    w = [BitVecVal(0, 32) for _ in range(64)]
    for i in range(16):
      upper = c.size() - (i * 32) - 1
      lower = c.size() - ((i + 1) * 32)
      w[i] = Extract(upper, lower, c)

    for i in range(16, 64):
      s0 = self.__rotr(w[i-15], 7) ^ self.__rotr(w[i-15], 18) ^ LShR(w[i-15], 3)

      part1 = self.__rotr(w[i-2], 17)
      part2 = self.__rotr(w[i-2], 19)
      part3 = LShR(w[i-2], 10)  # Z3 equivalent of w[i-2] >> 10

      s1 = part1 ^ part2 ^ part3
      w[i] = (w[i-16] + s0 + w[i-7] + s1) & _F32

    a, b, c, d, e, f, g, h = self.__h

    for i in range(64):
      s0 = self.__rotr(a, 2) ^ self.__rotr(a, 13) ^ self.__rotr(a, 22)
      t2 = s0 + self.__maj(a, b, c)
      s1 = self.__rotr(e, 6) ^ self.__rotr(e, 11) ^ self.__rotr(e, 25)
      t1 = h + s1 + self.__ch(e, f, g) + self.__k[i] + w[i]

      h = g
      g = f
      f = e
      e = (d + t1) & _F32
      d = c
      c = b
      b = a
      a = (t1 + t2) & _F32

    for i, (x, y) in enumerate(zip(self.__h, [a, b, c, d, e, f, g, h])):
      self.__h[i] = (x + y) & _F32


  def __update(self, bv):
    self.__counter += (bv.size() // 8)  # add number of bytes in the BitVector
    bv = Concat(self.__cache, bv) if self.__cache is not None else bv
    n = bv.size()

    for i in range(0, n // 512):
      upper = n - (i * 512) - 1
      lower = n - ((i + 1) * 512)
      extraction = Extract(upper, lower, bv)
      self.__compress(extraction)

    upper = max(0, bv.size() % 512 - 1)
    lower = 0
    self.__cache = None if upper == lower else Extract(upper, lower, bv)
