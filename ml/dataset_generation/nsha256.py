# -*- coding: utf-8 -*-
#!/usr/bin/python3

"""
Adapted from https://bitbucket.org/pypy/pypy/src/tip/lib_pypy/_sha256.py
"""

import hashlib
from BitVector import BitVector

from dataset_generation.sym_bit_vec import SymBitVec


SHA_BLOCKSIZE = 64
SHA_DIGESTSIZE = 32  # in bytes
F32 = SymBitVec(BitVector(intVal=0xffffffff, size=32))
F8  = SymBitVec(BitVector(intVal=0xff, size=32))

ROR = lambda x, y: (((x & F32) >> (y & 31)) | (x << (32 - (y & 31)))) & F32
Ch = lambda x, y, z: (z ^ (x & (y ^ z)))
Maj = lambda x, y, z: (((x | y) & z) | (x & y))
S = lambda x, n: ROR(x, n)
R = lambda x, n: (x & F32) >> n
Sigma0 = lambda x: (S(x, 2) ^ S(x, 13) ^ S(x, 22))
Sigma1 = lambda x: (S(x, 6) ^ S(x, 11) ^ S(x, 25))
Gamma0 = lambda x: (S(x, 7) ^ S(x, 18) ^ R(x, 3))
Gamma1 = lambda x: (S(x, 17) ^ S(x, 19) ^ R(x, 10))


def sha_transform(sha_info, difficulty):
  W = []

  d = [x.resize(32) for x in sha_info['data']]
  for i in range(0, 16):
    W.append((d[4 * i] << 24) + (d[4 * i + 1] << 16) + (d[4 * i + 2] << 8) + d[4 * i + 3])

  for i in range(16, 64):
    W.append((Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16]) & F32)

  ss = sha_info['digest'][:]

  def RND(a, b, c, d, e, f, g, h, i, ki):
    t0 = h + Sigma1(e) + Ch(e, f, g) + ki + W[i]
    t1 = Sigma0(a) + Maj(a, b, c)
    d += t0
    h  = t0 + t1
    return d & F32, h & F32

  def finish(ss):
    dig = []
    for i, x in enumerate(sha_info['digest']):
      dig.append((x + ss[i]) & F32)
    sha_info['digest'] = dig

  itr = 0

  ss[3], ss[7] = RND(ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],0,SymBitVec(BitVector(intVal=0x428a2f98, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[2], ss[6] = RND(ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],1,SymBitVec(BitVector(intVal=0x71374491, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[1], ss[5] = RND(ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],2,SymBitVec(BitVector(intVal=0xb5c0fbcf, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[0], ss[4] = RND(ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],3,SymBitVec(BitVector(intVal=0xe9b5dba5, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[7], ss[3] = RND(ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],4,SymBitVec(BitVector(intVal=0x3956c25b, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[6], ss[2] = RND(ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],5,SymBitVec(BitVector(intVal=0x59f111f1, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[5], ss[1] = RND(ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],6,SymBitVec(BitVector(intVal=0x923f82a4, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[4], ss[0] = RND(ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],7,SymBitVec(BitVector(intVal=0xab1c5ed5, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[3], ss[7] = RND(ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],8,SymBitVec(BitVector(intVal=0xd807aa98, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[2], ss[6] = RND(ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],9,SymBitVec(BitVector(intVal=0x12835b01, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[1], ss[5] = RND(ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],10,SymBitVec(BitVector(intVal=0x243185be, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[0], ss[4] = RND(ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],11,SymBitVec(BitVector(intVal=0x550c7dc3, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[7], ss[3] = RND(ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],12,SymBitVec(BitVector(intVal=0x72be5d74, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[6], ss[2] = RND(ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],13,SymBitVec(BitVector(intVal=0x80deb1fe, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[5], ss[1] = RND(ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],14,SymBitVec(BitVector(intVal=0x9bdc06a7, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[4], ss[0] = RND(ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],15,SymBitVec(BitVector(intVal=0xc19bf174, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[3], ss[7] = RND(ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],16,SymBitVec(BitVector(intVal=0xe49b69c1, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[2], ss[6] = RND(ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],17,SymBitVec(BitVector(intVal=0xefbe4786, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[1], ss[5] = RND(ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],18,SymBitVec(BitVector(intVal=0x0fc19dc6, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[0], ss[4] = RND(ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],19,SymBitVec(BitVector(intVal=0x240ca1cc, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[7], ss[3] = RND(ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],20,SymBitVec(BitVector(intVal=0x2de92c6f, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[6], ss[2] = RND(ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],21,SymBitVec(BitVector(intVal=0x4a7484aa, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[5], ss[1] = RND(ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],22,SymBitVec(BitVector(intVal=0x5cb0a9dc, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[4], ss[0] = RND(ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],23,SymBitVec(BitVector(intVal=0x76f988da, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[3], ss[7] = RND(ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],24,SymBitVec(BitVector(intVal=0x983e5152, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[2], ss[6] = RND(ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],25,SymBitVec(BitVector(intVal=0xa831c66d, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[1], ss[5] = RND(ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],26,SymBitVec(BitVector(intVal=0xb00327c8, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[0], ss[4] = RND(ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],27,SymBitVec(BitVector(intVal=0xbf597fc7, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[7], ss[3] = RND(ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],28,SymBitVec(BitVector(intVal=0xc6e00bf3, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[6], ss[2] = RND(ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],29,SymBitVec(BitVector(intVal=0xd5a79147, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[5], ss[1] = RND(ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],30,SymBitVec(BitVector(intVal=0x06ca6351, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[4], ss[0] = RND(ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],31,SymBitVec(BitVector(intVal=0x14292967, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[3], ss[7] = RND(ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],32,SymBitVec(BitVector(intVal=0x27b70a85, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[2], ss[6] = RND(ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],33,SymBitVec(BitVector(intVal=0x2e1b2138, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[1], ss[5] = RND(ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],34,SymBitVec(BitVector(intVal=0x4d2c6dfc, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[0], ss[4] = RND(ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],35,SymBitVec(BitVector(intVal=0x53380d13, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[7], ss[3] = RND(ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],36,SymBitVec(BitVector(intVal=0x650a7354, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[6], ss[2] = RND(ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],37,SymBitVec(BitVector(intVal=0x766a0abb, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[5], ss[1] = RND(ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],38,SymBitVec(BitVector(intVal=0x81c2c92e, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[4], ss[0] = RND(ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],39,SymBitVec(BitVector(intVal=0x92722c85, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[3], ss[7] = RND(ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],40,SymBitVec(BitVector(intVal=0xa2bfe8a1, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[2], ss[6] = RND(ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],41,SymBitVec(BitVector(intVal=0xa81a664b, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[1], ss[5] = RND(ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],42,SymBitVec(BitVector(intVal=0xc24b8b70, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[0], ss[4] = RND(ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],43,SymBitVec(BitVector(intVal=0xc76c51a3, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[7], ss[3] = RND(ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],44,SymBitVec(BitVector(intVal=0xd192e819, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[6], ss[2] = RND(ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],45,SymBitVec(BitVector(intVal=0xd6990624, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[5], ss[1] = RND(ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],46,SymBitVec(BitVector(intVal=0xf40e3585, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[4], ss[0] = RND(ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],47,SymBitVec(BitVector(intVal=0x106aa070, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[3], ss[7] = RND(ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],48,SymBitVec(BitVector(intVal=0x19a4c116, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[2], ss[6] = RND(ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],49,SymBitVec(BitVector(intVal=0x1e376c08, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[1], ss[5] = RND(ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],50,SymBitVec(BitVector(intVal=0x2748774c, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[0], ss[4] = RND(ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],51,SymBitVec(BitVector(intVal=0x34b0bcb5, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[7], ss[3] = RND(ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],52,SymBitVec(BitVector(intVal=0x391c0cb3, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[6], ss[2] = RND(ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],53,SymBitVec(BitVector(intVal=0x4ed8aa4a, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[5], ss[1] = RND(ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],54,SymBitVec(BitVector(intVal=0x5b9cca4f, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[4], ss[0] = RND(ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],55,SymBitVec(BitVector(intVal=0x682e6ff3, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[3], ss[7] = RND(ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],56,SymBitVec(BitVector(intVal=0x748f82ee, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[2], ss[6] = RND(ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],57,SymBitVec(BitVector(intVal=0x78a5636f, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[1], ss[5] = RND(ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],ss[5],58,SymBitVec(BitVector(intVal=0x84c87814, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[0], ss[4] = RND(ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],ss[4],59,SymBitVec(BitVector(intVal=0x8cc70208, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[7], ss[3] = RND(ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],ss[3],60,SymBitVec(BitVector(intVal=0x90befffa, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[6], ss[2] = RND(ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],ss[2],61,SymBitVec(BitVector(intVal=0xa4506ceb, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[5], ss[1] = RND(ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],ss[1],62,SymBitVec(BitVector(intVal=0xbef9a3f7, size=32)))
  itr += 1
  if itr == difficulty:
    finish(ss)
    return

  ss[4], ss[0] = RND(ss[1],ss[2],ss[3],ss[4],ss[5],ss[6],ss[7],ss[0],63,SymBitVec(BitVector(intVal=0xc67178f2, size=32)))
  finish(ss)


def sha_init():
  sha_info = dict()
  init_data = [0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19]
  sha_info['digest'] = [SymBitVec(BitVector(intVal=x, size=32)) for x in init_data]
  sha_info['count_lo'] = 0
  sha_info['count_hi'] = 0
  sha_info['local'] = 0
  sha_info['digestsize'] = 32
  sha_info['data'] = [SymBitVec(BitVector(intVal=0, size=8)) for _ in range(SHA_BLOCKSIZE)]
  return sha_info


def getbuf(input_data):
  if isinstance(input_data, SymBitVec):
    return input_data
  elif isinstance(input_data, BitVector):
    return SymBitVec(input_data, unknown=True)
  elif isinstance(input_data, str):
    b = bytes(input_data, 'utf-8')
    i = int.from_bytes(b, 'big')
    return SymBitVec(BitVector(intVal=i, size=len(b) * 8), unknown=True)
  else:
    raise NotImplementedError('SHA256: Unsupported input of type "{}"'.format(type(input_data)))


def sha_update(sha_info, bitvec, difficulty):
  count = len(bitvec) // 8  # we want it in bytes, `buffer` is a bit vector
  buffer_idx = 0
  clo = (sha_info['count_lo'] + (count << 3)) & 0xffffffff
  if clo < sha_info['count_lo']:
    sha_info['count_hi'] += 1
  sha_info['count_lo'] = clo

  sha_info['count_hi'] += (count >> 29)

  if sha_info['local']:
    i = SHA_BLOCKSIZE - sha_info['local']
    if i > count:
      i = count

    # copy buffer
    num_bytes = i
    for byte_idx in range(num_bytes):
      bit_lower = (buffer_idx + byte_idx) * 8
      bit_upper = (buffer_idx + byte_idx + 1) * 8
      sha_info['data'][sha_info['local'] + byte_idx] = bitvec.extract(bit_lower, bit_upper)

    count -= i
    buffer_idx += i

    sha_info['local'] += i
    if sha_info['local'] == SHA_BLOCKSIZE:
      sha_transform(sha_info, difficulty)
      sha_info['local'] = 0
    else:
      return

  while count >= SHA_BLOCKSIZE:
    # copy buffer
    sha_info['data'] = []
    for byte_idx in range(SHA_BLOCKSIZE):
      bit_lower = (buffer_idx + byte_idx) * 8
      bit_upper = (buffer_idx + byte_idx + 1) * 8
      sha_info['data'].append(bitvec.extract(bit_lower, bit_upper))

    count -= SHA_BLOCKSIZE
    buffer_idx += SHA_BLOCKSIZE
    sha_transform(sha_info, difficulty)

  # copy buffer
  pos = sha_info['local']
  for byte_idx in range(count):
    bit_lower = (buffer_idx + byte_idx) * 8
    bit_upper = (buffer_idx + byte_idx + 1) * 8
    sha_info['data'][pos + byte_idx] = bitvec.extract(bit_lower, bit_upper)

  sha_info['local'] = count


def sha_final(sha_info, difficulty):
  lo_bit_count = sha_info['count_lo']
  hi_bit_count = sha_info['count_hi']
  count = (lo_bit_count >> 3) & 0x3f
  sha_info['data'][count] = SymBitVec(BitVector(intVal=0x80, size=8))
  count += 1
  zero = SymBitVec(BitVector(intVal=0, size=8))

  if count > SHA_BLOCKSIZE - 8:
    # zero the bytes in data after the count
    sha_info['data'] = sha_info['data'][:count] + [zero for _ in range(SHA_BLOCKSIZE - count)]
    sha_transform(sha_info, difficulty)
    # zero bytes in data
    sha_info['data'] = [zero for _ in range(SHA_BLOCKSIZE)]
  else:
    sha_info['data'] = sha_info['data'][:count] + [zero for _ in range(SHA_BLOCKSIZE - count)]

  lo_bit_count = SymBitVec(BitVector(intVal=lo_bit_count, size=32))
  hi_bit_count = SymBitVec(BitVector(intVal=hi_bit_count, size=32))
  sha_info['data'][56] = (hi_bit_count >> 24).resize(8)
  sha_info['data'][57] = (hi_bit_count >> 16).resize(8)
  sha_info['data'][58] = (hi_bit_count >>  8).resize(8)
  sha_info['data'][59] = (hi_bit_count >>  0).resize(8)
  sha_info['data'][60] = (lo_bit_count >> 24).resize(8)
  sha_info['data'][61] = (lo_bit_count >> 16).resize(8)
  sha_info['data'][62] = (lo_bit_count >>  8).resize(8)
  sha_info['data'][63] = (lo_bit_count >>  0).resize(8)

  sha_transform(sha_info, difficulty)

  dig = []
  for i in sha_info['digest']:
    dig += [(i >> 24) & F8, (i >> 16) & F8, (i >> 8) & F8, i & F8]
  return dig


class sha256(object):

  def __init__(self, input_data=None, difficulty=64):
    self._sha = sha_init()
    self._difficulty = difficulty

    if input_data is None:
      self._real_hasher = hashlib.sha256()
      return

    input_data = getbuf(input_data)
    sha_update(self._sha, input_data, self._difficulty)

    int_val = int(input_data)
    input_bytes = int_val.to_bytes((int_val.bit_length() + 7) // 8, 'big')
    self._real_hasher = hashlib.sha256(input_bytes)

  def update(self, input_data):
    input_data = getbuf(input_data)
    sha_update(self._sha, input_data, self._difficulty)

    int_val = int(input_data)
    input_bytes = int_val.to_bytes((int_val.bit_length() + 7) // 8, 'big')
    self._real_hasher.update(input_bytes)

  def hexdigest(self):
    return ''.join(['%.2x' % int(i) for i in self.bitvecDigest()])

  def bitvecDigest(self):
    result = sha_final(self._sha.copy(), self._difficulty)[:self._sha['digestsize']]
    if self._difficulty == 64:
      dig = ''.join(['%.2x' % int(i) for i in result])
      real_dig = self._real_hasher.hexdigest()
      if dig != real_dig:
        raise RuntimeError('Expected {}, got {}'.format(real_dig, dig))
    return result


def test():
  a_str = 'just a test string'

  assert 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855' == sha256().hexdigest()
  assert 'd7b553c6f09ac85d142415f857c5310f3bbbe7cdd787cce4b985acedd585266f' == sha256(a_str).hexdigest()
  assert '8113ebf33c97daa9998762aacafe750c7cefc2b2f173c90c59663a57fe626f21' == sha256(a_str * 7).hexdigest()

  a_str_bytes = bytes(a_str, 'utf-8')
  result = sha256(BitVector(intVal=int.from_bytes(a_str_bytes, 'big'), size=len(a_str) * 8)).hexdigest()
  assert 'd7b553c6f09ac85d142415f857c5310f3bbbe7cdd787cce4b985acedd585266f' == result

  s = sha256(a_str)
  s.update(a_str)
  assert '03d9963e05a094593190b6fc794cb1a3e1ac7d7883f0b5855268afeccc70d461' == s.hexdigest()

if __name__ == '__main__':
  test()
