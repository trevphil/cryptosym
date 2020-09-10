# -*- coding: utf-8 -*-
import unittest
import random
import hashlib
import codecs
from random import randint

import nsha256
from nsha256_reverse import NSHA256Reverse


class TestHelper():
  def generate_random_string(self, str_len=None):
    s = ''
    str_len = str_len if str_len is not None else randint(0, 200)

    for i in range(0, str_len):
      if randint(0, 1) == 0:
        s += chr(randint(97, 122)) # a-z
      else:
        s += chr(randint(65, 90)) # A-Z

    return s


class TestPadding(unittest.TestCase):
  def test_fixed(self):
    self.assertEqual(len(nsha256.pad(0)), 64)
    self.assertEqual(len(nsha256.pad(10)), 54)
    self.assertEqual(len(nsha256.pad(100)), 28)
    self.assertEqual(len(nsha256.pad(1000)), 24)


class TestAgainstHashlib(unittest.TestCase):
  def setUp(self):
    self.TH = TestHelper()

  def test_block_hash(self):
    # https://en.bitcoin.it/wiki/Block_hashing_algorithm
    header_hex = (
      "01000000" + # Block version number
      "81cd02ab7e569e8bcd9317e2fe99f2de44d49ab2b8851ba4a308000000000000" + # Prev. block hash
      "e320b6c2fffc8d750423db8b1eb942ae710e951ed797f7affc8892b0f1fc122b" + # Merkle root
      "c7f5d74d" + # Block timestamp as seconds since 1970-01-01T00:00 UTC
      "f2b9441a" + # Bits (current target in compact format)
      "42a14695" # Nonce (32 bits)
    )

    header_bytes = codecs.decode(header_hex, 'hex')
    first_digest = hashlib.sha256(header_bytes).digest()
    first_hash = hashlib.sha256(header_bytes).hexdigest()

    n = nsha256.new()
    n.update(header_bytes)
    self.assertEqual(first_hash, n.hexdigest())

    second_hash = hashlib.sha256(first_digest).hexdigest()[::-1] # reverse the order
    expected = '1dbd981fe6985776b644b173a4d0385ddc1aa2a829688d1e0000000000000000'[::-1]
    self.assertEqual(second_hash, expected)

    n = nsha256.new()
    n.update(first_digest)
    self.assertEqual(second_hash, n.hexdigest()[::-1])


class TestReverseImplementation(unittest.TestCase):
  def test_block_hash_reversal(self):
    # https://en.bitcoin.it/wiki/Block_hashing_algorithm
    header_no_nonce = (
      "01000000" + # Block version number
      "81cd02ab7e569e8bcd9317e2fe99f2de44d49ab2b8851ba4a308000000000000" + # Prev. block hash
      "e320b6c2fffc8d750423db8b1eb942ae710e951ed797f7affc8892b0f1fc122b" + # Merkle root
      "c7f5d74d" + # Block timestamp as seconds since 1970-01-01T00:00 UTC
      "f2b9441a"   # Bits (current target in compact format)
      # "42a14695" # Nonce (32 bits)
    )
    big_endian_hash = '1dbd981fe6985776b644b173a4d0385ddc1aa2a829688d1e0000000000000000'

    reverser = NSHA256Reverse(
      block_header_no_nonce=int(header_no_nonce, 16),
      block_hash=big_endian_hash)

    success, nonce, hash_guess = reverser.reverse_hash256()
    print('Finished.\n\tsuccess: {}\n\tnonce: {}\n\thash guess: {}'\
      .format(success, nonce, hash_guess))
    self.assertTrue(success)
    self.assertEqual(hash_guess, big_endian_hash)


if __name__ == '__main__':
  unittest.main()
