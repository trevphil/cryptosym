# -*- coding: utf-8 -*-
import codecs
from z3 import *

_MD5_HASH_LEN = 128 # bits
_ONE = BitVecVal(1, 1)


class NMD5Reverse:
  """Reverse-engineer MD5 hash function"""

  def __init__(self, leading_zero_bits, input_message_len):
    assert leading_zero_bits < _MD5_HASH_LEN, 'Too many leading zeros'

    self.__A = 0x67452301
    self.__B = 0xEFCDAB89
    self.__C = 0x98BADCFE
    self.__D = 0x10325476

    self.__solver = Solver()
    self.__leading_zero_bits = leading_zero_bits
    self.__input_message_len = input_message_len
    self.__input_guess = BitVec('hash_input_guess', input_message_len)
    self.__hash_guess = BitVec('hash_guess', _MD5_HASH_LEN)

  ## Public class methods

  def reverse(self):
    # Run algorithm and update internal state of registers
    self.__compute()

    # Compute the (symbolic) digest as a function of the input guess variable
    n = _MD5_HASH_LEN
    digest_bits = self.digest()
    assert digest_bits.size() == n, 'Wrong number of digest bits for an MD5 hash'

    # Constrain the hash guess variable to be equal to the output from
    # hashing the input guess variable (it's just symbolic constraints).
    self.__solver.add(self.__hash_guess == digest_bits)

    # Enforce that a certain number of leading bits in the hash must be zero.
    # This also has an effect on the possible values the hash input can be.
    leading = Extract(n - 1, n - self.__leading_zero_bits, self.__hash_guess)
    self.__solver.add(leading == 0)

    # Now what we do is fix the beginning of the hash input message to some arbitrary
    # value (for example, zero) which leaves only a few bits of the hash input
    # as a "free" variable which can be chosen by the solver to achieve the
    # constraints on the hash guess, namely that it has a certain number of leading 0's.
    n = self.__input_message_len
    input_beginning = Extract(n - 1, 64, self.__input_guess) # Leave 64 variable bits for nonce
    self.__solver.add(input_beginning == 0) # Arbitrary value, choose 0 for simplicity

    # Check if the constraints are satisfiable
    status = self.__solver.check()
    print('\nSatisfiability = {}'.format(status))

    if status == sat:
      model = self.__solver.model()
      print('\nModel:\n{}\n'.format(model))

      # Print the model
      input_guess = model[self.__input_guess].as_long()
      input_guess = hex(input_guess)[2:].replace('L', '')
      input_guess = codecs.decode(input_guess, 'hex')

      hash_guess = model[self.__hash_guess].as_long()
      hash_guess = bin(hash_guess)[2:]
      hash_guess = '0' * (_MD5_HASH_LEN - len(hash_guess)) + hash_guess

      return (True, input_guess, hash_guess)

    return (False, None, None)

  def digest(self):
    """Returns the byte string digest. The idea is to
    slice every register into 4 bytes and go from the
    low order bytes of A to the high order bytes of D."""

    result = None
    bit_vec_buffers = [self.__A, self.__B, self.__C, self.__D]

    for bit_vec in bit_vec_buffers:
      bufferbytes = []
      if bit_vec.size() % 32 != 0:
        zeros = BitVecVal(0, bit_vec.size() % 32)
        bit_vec = Concat(zeros, bit_vec) # pad leading zero if missing

      # Extract(upper_bound, lower_bound, bit_vector)
      bufferbytes.append(Extract(7,   0,  bit_vec))
      bufferbytes.append(Extract(15,  8,  bit_vec))
      bufferbytes.append(Extract(23,  16, bit_vec))
      bufferbytes.append(Extract(31,  24, bit_vec))

      result = bufferbytes[0] if result is None else Concat(result, bufferbytes[0])
      result = Concat(result, bufferbytes[1])
      result = Concat(result, bufferbytes[2])
      result = Concat(result, bufferbytes[3])

    return result

  # Main hashing function
  def __compute(self):
    chunks = self.__splitToBlocks(self.__pad(self.__input_guess), 512)

    R11, R12, R13, R14 = 7, 12, 17, 22
    R21, R22, R23, R24 = 5, 9, 14, 20
    R31, R32, R33, R34 = 4, 11, 16, 23
    R41, R42, R43, R44 = 6, 10, 15, 21

    F, G, H, I, R, = self.__F, self.__G, self.__H, self.__I, self.__R

    for idx, chunk in enumerate(chunks):
      words = self.__createWordArray(chunk, self.__input_message_len // 8, idx == len(chunks)-1)

      a, b, c, d = A, B, C, D = self.__A, self.__B, self.__C, self.__D

      # Round 1
      a = R(F, a, b, c, d, words[ 0], R11, 0xD76AA478)
      d = R(F, d, a, b, c, words[ 1], R12, 0xE8C7B756)
      c = R(F, c, d, a, b, words[ 2], R13, 0x242070DB)
      b = R(F, b, c, d, a, words[ 3], R14, 0xC1BDCEEE)
      a = R(F, a, b, c, d, words[ 4], R11, 0xF57C0FAF)
      d = R(F, d, a, b, c, words[ 5], R12, 0x4787C62A)
      c = R(F, c, d, a, b, words[ 6], R13, 0xA8304613)
      b = R(F, b, c, d, a, words[ 7], R14, 0xFD469501)
      a = R(F, a, b, c, d, words[ 8], R11, 0x698098D8)
      d = R(F, d, a, b, c, words[ 9], R12, 0x8B44F7AF)
      c = R(F, c, d, a, b, words[10], R13, 0xFFFF5BB1)
      b = R(F, b, c, d, a, words[11], R14, 0x895CD7BE)
      a = R(F, a, b, c, d, words[12], R11, 0x6B901122)
      d = R(F, d, a, b, c, words[13], R12, 0xFD987193)
      c = R(F, c, d, a, b, words[14], R13, 0xA679438E)
      b = R(F, b, c, d, a, words[15], R14, 0x49B40821)

      # Round 2
      a = R(G, a, b, c, d, words[ 1], R21, 0xF61E2562)
      d = R(G, d, a, b, c, words[ 6], R22, 0xC040B340)
      c = R(G, c, d, a, b, words[11], R23, 0x265E5A51)
      b = R(G, b, c, d, a, words[ 0], R24, 0xE9B6C7AA)
      a = R(G, a, b, c, d, words[ 5], R21, 0xD62F105D)
      d = R(G, d, a, b, c, words[10], R22, 0x02441453)
      c = R(G, c, d, a, b, words[15], R23, 0xD8A1E681)
      b = R(G, b, c, d, a, words[ 4], R24, 0xE7D3FBC8)
      a = R(G, a, b, c, d, words[ 9], R21, 0x21E1CDE6)
      d = R(G, d, a, b, c, words[14], R22, 0xC33707D6)
      c = R(G, c, d, a, b, words[ 3], R23, 0xF4D50D87)
      b = R(G, b, c, d, a, words[ 8], R24, 0x455A14ED)
      a = R(G, a, b, c, d, words[13], R21, 0xA9E3E905)
      d = R(G, d, a, b, c, words[ 2], R22, 0xFCEFA3F8)
      c = R(G, c, d, a, b, words[ 7], R23, 0x676F02D9)
      b = R(G, b, c, d, a, words[12], R24, 0x8D2A4C8A)

      # Round 3
      a = R(H, a, b, c, d, words[ 5], R31, 0xFFFA3942)
      d = R(H, d, a, b, c, words[ 8], R32, 0x8771F681)
      c = R(H, c, d, a, b, words[11], R33, 0x6D9D6122)
      b = R(H, b, c, d, a, words[14], R34, 0xFDE5380C)
      a = R(H, a, b, c, d, words[ 1], R31, 0xA4BEEA44)
      d = R(H, d, a, b, c, words[ 4], R32, 0x4BDECFA9)
      c = R(H, c, d, a, b, words[ 7], R33, 0xF6BB4B60)
      b = R(H, b, c, d, a, words[10], R34, 0xBEBFBC70)
      a = R(H, a, b, c, d, words[13], R31, 0x289B7EC6)
      d = R(H, d, a, b, c, words[ 0], R32, 0xEAA127FA)
      c = R(H, c, d, a, b, words[ 3], R33, 0xD4EF3085)
      b = R(H, b, c, d, a, words[ 6], R34, 0x04881D05)
      a = R(H, a, b, c, d, words[ 9], R31, 0xD9D4D039)
      d = R(H, d, a, b, c, words[12], R32, 0xE6DB99E5)
      c = R(H, c, d, a, b, words[15], R33, 0x1FA27CF8)
      b = R(H, b, c, d, a, words[ 2], R34, 0xC4AC5665)

      # Round 4
      a = R(I, a, b, c, d, words[ 0], R41, 0xF4292244)
      d = R(I, d, a, b, c, words[ 7], R42, 0x432AFF97)
      c = R(I, c, d, a, b, words[14], R43, 0xAB9423A7)
      b = R(I, b, c, d, a, words[ 5], R44, 0xFC93A039)
      a = R(I, a, b, c, d, words[12], R41, 0x655B59C3)
      d = R(I, d, a, b, c, words[ 3], R42, 0x8F0CCC92)
      c = R(I, c, d, a, b, words[10], R43, 0xFFEFF47D)
      b = R(I, b, c, d, a, words[ 1], R44, 0x85845DD1)
      a = R(I, a, b, c, d, words[ 8], R41, 0x6FA87E4F)
      d = R(I, d, a, b, c, words[15], R42, 0xFE2CE6E0)
      c = R(I, c, d, a, b, words[ 6], R43, 0xA3014314)
      b = R(I, b, c, d, a, words[13], R44, 0x4E0811A1)
      a = R(I, a, b, c, d, words[ 4], R41, 0xF7537E82)
      d = R(I, d, a, b, c, words[11], R42, 0xBD3AF235)
      c = R(I, c, d, a, b, words[ 2], R43, 0x2AD7D2BB)
      b = R(I, b, c, d, a, words[ 9], R44, 0xEB86D391)

      A = (a + A) & 0xffffffff
      B = (b + B) & 0xffffffff
      C = (c + C) & 0xffffffff
      D = (d + D) & 0xffffffff

      self.__A = A
      self.__B = B
      self.__C = C
      self.__D = D


  ## Private class methods

  def __toBinaryString(self, string):
    """Converts a given string into a binary representation of itself"""

    return ''.join("{:08b}".format(byte) for byte in bytearray(string.encode('utf-8')))

  def __pad(self, input_bit_vector):
    """Adds padding to bit vector be congruent to 448 mod 512"""
    messageLength = input_bit_vector.size()

    input_bit_vector = Concat(input_bit_vector, _ONE)

    n = input_bit_vector.size()
    if n % 512 != 448:
      pad = 448 - n if n < 448 else 512 + 448 - n
      input_bit_vector = Concat(input_bit_vector, BitVecVal(0, pad))

    padding_str = self.__pad64B(messageLength)
    padding = BitVecVal(int(padding_str, 2), len(padding_str))
    return Concat(input_bit_vector, padding)

  def __pad64B(self, length):
    """Creates a little-endian 64-bit representation of the message length"""
    s = bin(length).replace('b', '0')

    # If we reach 64-bit overflow
    if len(s) > 64:
      return '0' + '1'*63

    padded = "0" * (64 - len(s))
    padded += s[::-1] # reverse length byte first to preserve correct order
    return padded[::-1]


  def __splitToBlocks(self, message, n):
    """Helper method to split a message into equal sized parts.
    Use for 512-bit sized 'blocks' or 16-bit sized 'words'."""

    # index 0 of message is right-most bit (LSB)
    s = message.size()

    # Extract(upper_bound, lower_bound, bit_vector)
    return [Extract(s - i - 1, s - (i + n), message) for i in range(0, s, n)]

  def __createWordArray(self, message, messageLength, finalBlock):
    """Primes the 16-bit words for the main function and returns as an
    array. Repeated calls with the finalBlock parameter will determine if we
    need to put the length and 0 last."""

    # CRITICAL!!!
    # Although we operate on "8-bit bytes", we actually need to support much bigger numbers
    N = 64

    message = self.__splitToBlocks(message, 32)
    wordArray = [BitVecVal(0, N)] * 16

    wordIndex = 0
    for word in message:
      bytes = self.__splitToBlocks(word, 8)
      powers = 0

      for byte in bytes:
        if byte.size() != N:
          zeros = BitVecVal(0, N - byte.size())
          byte = Concat(zeros, byte)

        tempByte = wordArray[wordIndex]
        tempByte = tempByte | (byte << powers)
        powers += 8
        wordArray[wordIndex] = tempByte

      wordIndex += 1
      powers = 0

    ## correct last two bytes if we're on the final block
    if finalBlock:
      wordArray[-2] = BitVecVal(messageLength << 3, N)
      wordArray[-1] = BitVecVal(messageLength >> 29, N)

    return [Extract(32, 0, x) for x in wordArray]

  def __F(self, x, y, z):
    """XY v not(X) Z"""
    return (x & y) | ((~x) & z)

  def __G(self, x, y, z):
    """XZ v Y not(Z)"""
    return (x & z) | (y & (~z))

  def __H(self, x, y, z):
    """X xor Y xor Z"""
    return x ^ y ^ z

  def __I(self, x, y, z):
    """Y xor (X v not(Z))"""
    return y ^ (x | (~z))

  def __rotateLeft(self, x, n):
    """Rotates x left by n (note that Python bitshift is arithmetic)"""
    return (x << n) | (x >> (32-n))

  def __R(self, function,a,b,c,d,x,s,ac):
    """Wrapper function for the MD5 rounds"""

    r = a + function(b,c,d)
    r = self.__rotateLeft((r + x + ac) & 0xffffffff, s)
    return (r + b) & 0xffffffff # Keep r unsigned

## Public methods for module

def new(leading_zero_bits, input_message_len):
  """Create a new NMD5Reverse object"""
  return NMD5Reverse(leading_zero_bits, input_message_len)

def md5_reverse(leading_zero_bits, input_message_len):
  """Create a new NMD5Reverse object"""
  return NMD5Reverse(leading_zero_bits, input_message_len)
