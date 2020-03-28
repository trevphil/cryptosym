# -*- coding: utf-8 -*-
import nmd5
import nmd5_reverse
import unittest
import random
from lib.linkedlist import *
from random import randint

"""
The first tests in this file test the normal MD5 implementation,
which is from https://github.com/narkkil/md5

The final tests are for the MD5 reversal, from me :)
"""

class TestHelper():
  def generate_random_binary(self):
    b = ''
    for _ in range(0, randint(0, 500)):
      x = str(randint(0, 1))
      b += x
    return b

  def bstring_formatter(self, string):
    string = bin(ord(string))

    if len(string) == 9:
      return string.replace('b', '')
    else:
      return string.replace('b', '0')


class TestBinaryStringConversion(unittest.TestCase):
  def setUp(self):
    self.TH = TestHelper()
    self.m = nmd5.new()

  def test_bstring_a(self):
    s = 'a'
    self.assertEqual(self.m._NMD5__toBinaryString(s), self.TH.bstring_formatter('a'))

  def test_bstring_b(self):
    s = 'b'
    self.assertEqual(self.m._NMD5__toBinaryString(s), self.TH.bstring_formatter('b'))


class TestPadding(unittest.TestCase):
  def setUp(self):
    self.TH = TestHelper()
    self.m = nmd5.new()

  def test_random(self):
    for x in range(0, 1000): # make sure edge cases are tested
      s = self.TH.generate_random_binary()
      self.assertTrue(len(self.m._NMD5__pad(s)) % 512 == 0)

  def test_fixed(self):
    s = '011000010111001101100100' #asd
    self.assertEqual(len(self.m._NMD5__pad(s)), 512)


class TestLinkedList(unittest.TestCase):
  def setUp(self):
    self.emptyList = LinkedList(None)
    self.initList = LinkedList(Node("Test", None))

  def test_empty_list(self):
    self.assertEqual("", self.emptyList.toString())

  def test_addition(self):
    self.emptyList.add(Node("Abc", None))
    self.assertEqual("Abc", self.emptyList.toString())

  def test_add_many(self):
    self.emptyList.add(Node("Abc", Node("def", Node("ghi", Node("klm", None)))))
    self.assertEqual("Abcdefghiklm", self.emptyList.toString())

  def test_init_list(self):
    self.assertEqual("Test", self.initList.toString())


class TestBlockSplit(unittest.TestCase):
  def setUp(self):
    self.m = nmd5.new()
    self.t1 = self.m._NMD5__pad(self.m._NMD5__toBinaryString("test1"))
    self.t2 = self.m._NMD5__pad(self.m._NMD5__toBinaryString("a"*66))

  def test_one_block(self):
    self.assertEqual(len(self.t1), 512)

  def test_another_block(self):
    self.assertEqual(len(self.t2), 1024)

  def test_split_32(self):
    self.assertEqual(len(self.m._NMD5__splitToBlocks(self.t1, 512)), 1)

  def test_split_32_2(self):
    self.assertEqual(len(self.m._NMD5__splitToBlocks(self.t2, 512)), 2)

  def test_split_32_3(self):
    split = self.m._NMD5__splitToBlocks(self.t1, 16)

    self.assertEqual(len(split), 32)


class TestCopy(unittest.TestCase):
  def setUp(self):
    self.m = nmd5.new("asd")
    self.n = self.m.copy()

  def test_copy_digests(self):
    self.assertEqual(self.m.digest(), self.n.digest())
    self.assertEqual(self.m.hexdigest(), self.n.hexdigest())


class TestAgainstHashlib(unittest.TestCase):
  def setUp(self):
    import nmd5
    import hashlib
    import random
    self.m = hashlib.md5()
    self.n = nmd5.new()

  def test_random_strings_digest(self):
    for i in range(0, 50):
      s = ('%06x' % random.randrange(16**6))
      if i % 6 == 0:
        s += 'öäå'
      self.m.update(s.encode('utf-8'))
      self.n.update(s)
      self.assertEqual(self.m.digest(), self.n.digest())
      self.assertEqual(self.m.hexdigest(), self.n.hexdigest())

  def test_long_string_digest(self):
    string = "abc"*70000
    self.m.update(string.encode('utf-8'))
    self.n.update(string)
    self.assertEqual(self.m.digest(), self.n.digest())
    self.assertEqual(self.m.hexdigest(), self.n.hexdigest())


class TestReverseImplementation(unittest.TestCase):
  def setUp(self):
    self.leading_zeros = 9
    self.reverser = nmd5_reverse.new(
      leading_zero_bits=self.leading_zeros,  # leading zeros in hash
      input_message_len=128)  # length of the message being hashed

  def test_md5_reversal(self):
    success, input_guess, hash_guess = self.reverser.reverse()
    print('Finished.\n\tsuccess: {}\n\tinput guess: {}\n\thash guess: {}'\
      .format(success, input_guess, hash_guess))
    self.assertTrue(success)
    self.assertEqual(hash_guess[:self.leading_zeros], '0' * self.leading_zeros)

"""

Here are some statistics about how changing the input message length
and the number of leading zeros affects runtime of the algorithm.

---- Constant block size (128 bits) ----
Leading zeros   Run-time
8               88 s
9               42 s
10              186 s
11              262 s
12              204 s
13              143 s
14              783 s
15              974 s
16              [a long time]


---- Constant leading zeros (8) ----
Block size      Run-time
128 bits        78 s
256 bits        71 s
512 bits        75 s
1024 bits       Overflow encountered when expanding vector

"""

if __name__ == '__main__':
  unittest.main()
