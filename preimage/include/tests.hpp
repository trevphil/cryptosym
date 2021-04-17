/*
 * Hash reversal
 *
 * Copyright (c) 2020 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#pragma once

#include <spdlog/spdlog.h>
#include <boost/dynamic_bitset.hpp>
#include <boost/algorithm/string.hpp>

#include <cryptopp/dll.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/ripemd.h>

#include <map>
#include <vector>
#include <string>
#include <assert.h>

#include "core/factor.hpp"
#include "core/sym_bit_vec.hpp"
#include "core/utils.hpp"
#include "hashing/hash_funcs.hpp"
#include "cmsat/cmsat_solver.hpp"
#include "bp/bp_solver.hpp"

namespace preimage {

void simpleTests() {
  boost::dynamic_bitset<> bits(64, 0xDEADBEEF);
  SymBitVec bv1(bits);
  SymBitVec bv2(0xDEADBEEF, 64);
  assert(bv1.intVal() == uint64_t(0xDEADBEEF));
  assert(bv1.intVal() == bv2.intVal());
  assert(Utils::hexstr(bits).compare("00000000deadbeef") == 0);
  assert(bv1.hex().compare("00000000deadbeef") == 0);
  assert(bv2.hex().compare("00000000deadbeef") == 0);
  spdlog::info("Simple tests passed.");
}

void symBitVecTests() {
  SymBitVec bv1(0b110101, 6);
  assert(bv1.rotr(2).intVal() == 0b010111);
  assert((bv1 >> 3).intVal() == 0b000110);
  assert((bv1 << 3).intVal() == 0b101000);
  assert((~bv1).intVal() == 0b001010);
  assert(bv1.extract(1, 5).intVal() == 0b1010);
  SymBitVec bv2(0b011101, 6);
  assert((bv1 ^ bv2).intVal() == 0b101000);
  assert((bv1 & bv2).intVal() == 0b010101);
  assert((bv1 | bv2).intVal() == 0b111101);
  assert((bv1 + bv2).intVal() == 0b010010);
  SymBitVec bv1_bigger = bv1.resize(10);
  assert(bv1_bigger.size() == 10);
  assert(bv1_bigger.intVal() == 0b0000110101);
  SymBitVec bv1_smaller = bv1.resize(2);
  assert(bv1_smaller.size() == 2);
  assert(bv1_smaller.intVal() == 0b01);
  SymBitVec t0(0, 32);
  SymBitVec t1(0b11010100010010100110100011100000, 32);
  SymBitVec summed = t0 + t1;
  assert(t1.intVal() == summed.intVal());

  spdlog::info("SymBitVec tests passed.");
}

void sha256Tests() {
  std::string empty = "";
  std::string s = "just a test string";
  std::string s7 = s + s + s + s + s + s + s;
  std::string h_empty =
      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
  std::string h_s =
      "d7b553c6f09ac85d142415f857c5310f3bbbe7cdd787cce4b985acedd585266f";
  std::string h_s7 =
      "8113ebf33c97daa9998762aacafe750c7cefc2b2f173c90c59663a57fe626f21";

  SHA256 sha256;

  std::string h = sha256.call(Utils::str2bits(empty)).hex();
  assert(h.compare(h_empty) == 0);

  h = sha256.call(Utils::str2bits(s)).hex();
  assert(h.compare(h_s) == 0);

  h = sha256.call(Utils::str2bits(s7)).hex();
  assert(h.compare(h_s7) == 0);

  Utils::seed(1);
  const std::vector<size_t> inp_sizes{0, 8, 32, 64, 512, 640, 1024};
  for (size_t inp_size : inp_sizes) {
    for (size_t sample_idx = 0; sample_idx < 10; sample_idx++) {
      // Generate a random input of length "inp_size"
      const boost::dynamic_bitset<> bits = Utils::randomBits(inp_size);
      const size_t n_bytes = inp_size / 8;
      uint8_t byte_arr[n_bytes];
      for (size_t byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
        byte_arr[byte_idx] = 0;
        for (size_t k = 0; k < 8; k++) {
          byte_arr[byte_idx] += bits[(byte_idx * 8) + k] << k;
        }
      }

      // Call SHA256 using Crypto++
      CryptoPP::SHA256 cryptopp_sha256;
      uint8_t digest[CryptoPP::SHA256::DIGESTSIZE];
      cryptopp_sha256.CalculateDigest(digest, byte_arr, n_bytes);

      // Convert output to lowercase hexadecimal
      CryptoPP::HexEncoder encoder;
      std::string expected_output;
      encoder.Attach(new CryptoPP::StringSink(expected_output));
      encoder.Put(digest, sizeof(digest));
      encoder.MessageEnd();
      boost::algorithm::to_lower(expected_output);

      // Call SHA256 using custom hash function
      h = sha256.call(bits).hex();

      if (h.compare(expected_output) != 0) {
        spdlog::info("inp_size={}, sample={}\n\tInput:\t{}\n\tExpected:\t{}\n\tGot:\t\t{}",
                     inp_size, sample_idx, Utils::hexstr(bits), expected_output, h);
      }
    }
  }

  spdlog::info("SHA256 tests passed, mean runtime: ~{:.0f} ms", sha256.averageRuntimeMs());
}

void ripemd160Tests() {
  RIPEMD160 ripemd160;

  Utils::seed(1);
  const std::vector<size_t> inp_sizes{0, 8, 32, 64, 512, 640, 1024};
  for (size_t inp_size : inp_sizes) {
    for (size_t sample_idx = 0; sample_idx < 10; sample_idx++) {
      // Generate a random input of length "inp_size" bits
      const boost::dynamic_bitset<> bits = Utils::randomBits(inp_size);
      const size_t n_bytes = inp_size / 8;
      uint8_t byte_arr[n_bytes];
      for (size_t byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
        byte_arr[byte_idx] = 0;
        for (size_t k = 0; k < 8; k++) {
          byte_arr[byte_idx] += bits[(byte_idx * 8) + k] << k;
        }
      }

      // Call RIPEMD160 using Crypto++
      CryptoPP::RIPEMD160 cryptopp_ripemd160;
      uint8_t digest[CryptoPP::RIPEMD160::DIGESTSIZE];
      cryptopp_ripemd160.CalculateDigest(digest, byte_arr, n_bytes);

      // Convert output to lowercase hexadecimal
      CryptoPP::HexEncoder encoder;
      std::string expected_output;
      encoder.Attach(new CryptoPP::StringSink(expected_output));
      encoder.Put(digest, sizeof(digest));
      encoder.MessageEnd();
      boost::algorithm::to_lower(expected_output);

      // Call RIPEMD160 using custom hash function
      const std::string h = ripemd160.call(bits).hex();

      if (h.compare(expected_output) != 0) {
        spdlog::info("inp_size={}, sample={}\n\tInput:\t{}\n\tExpected:\t{}\n\tGot:\t\t{}",
                     inp_size, sample_idx, Utils::hexstr(bits), expected_output, h);
        assert(false);
      }
    }
  }

  spdlog::info("RIPEMD160 tests passed, mean runtime: ~{:.0f} ms", ripemd160.averageRuntimeMs());
}

void md5Tests() {
  MD5 md5;

  Utils::seed(1);
  const std::vector<size_t> inp_sizes{0, 8, 32, 64, 512, 640, 1024};
  for (size_t inp_size : inp_sizes) {
    for (size_t sample_idx = 0; sample_idx < 10; sample_idx++) {
      // Generate a random input of length "inp_size" bits
      const boost::dynamic_bitset<> bits = Utils::randomBits(inp_size);
      const size_t n_bytes = inp_size / 8;
      uint8_t byte_arr[n_bytes];
      for (size_t byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
        byte_arr[byte_idx] = 0;
        for (size_t k = 0; k < 8; k++) {
          byte_arr[byte_idx] += bits[(byte_idx * 8) + k] << k;
        }
      }

      // Call MD5 using Crypto++
      CryptoPP::Weak::MD5 cryptopp_md5;
      uint8_t digest[CryptoPP::Weak::MD5::DIGESTSIZE];
      cryptopp_md5.CalculateDigest(digest, byte_arr, n_bytes);

      // Convert output to lowercase hexadecimal
      CryptoPP::HexEncoder encoder;
      std::string expected_output;
      encoder.Attach(new CryptoPP::StringSink(expected_output));
      encoder.Put(digest, sizeof(digest));
      encoder.MessageEnd();
      boost::algorithm::to_lower(expected_output);

      // Call MD5 using custom hash function
      const std::string h = md5.call(bits).hex();

      if (h.compare(expected_output) != 0) {
        spdlog::info("inp_size={}, sample={}\n\tInput:\t{}\n\tExpected:\t{}\n\tGot:\t\t{}",
                     inp_size, sample_idx, Utils::hexstr(bits), expected_output, h);
        assert(false);
      }
    }
  }

  spdlog::info("MD5 tests passed, mean runtime: ~{:.0f} ms", md5.averageRuntimeMs());
}

void bitcoinBlockTest() {
  // const std::vector<bool> block = {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  // const std::vector<bool> block = {1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  const std::vector<bool> block = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1};
  boost::dynamic_bitset bits(block.size());
  for (size_t i = 0; i < bits.size(); ++i) bits[i] = 1; // block.at(i);
  spdlog::info("Block size: {}", block.size());

  SHA256 sha256;
  const int difficulty = 64;
  const SymBitVec h1 = sha256.call(bits, difficulty);
  const SymBitVec h2 = sha256.call(h1.bits(), difficulty);
  spdlog::info(h1.hex());
  spdlog::info(h2.hex());
  // const std::string h2 = sha256.call(h1.bits(), difficulty).reversed().hex();
  // spdlog::info(h2);
}

void cmsatTests() {
  SHA256 sha256;
  const int difficulty = 4;  // Normal SHA256 has 64 rounds
  const boost::dynamic_bitset<> bits = Utils::randomBits(64, 1);
  SymBitVec h = sha256.call(bits, difficulty);

  // Collect observed bits
  std::map<size_t, bool> observed;
  for (size_t i = 0; i < h.size(); ++i) {
    const Bit &b = h.at(i);
    if (b.is_rv) observed[b.index] = b.val;
  }

  // Collect factors in a mapping from RV index --> factor
  const size_t n = Factor::global_factors.size();
  std::map<size_t, Factor> factors;
  for (size_t i = 0; i < n; ++i) {
    const Factor &f = Factor::global_factors.at(i);
    assert(i == f.output);
    // Skip bits which are not ancestors of observed bits
    if (!sha256.canIgnore(i)) factors[i] = f;
  }

  CMSatSolver solver(factors, sha256.hashInputIndices());
  const std::map<size_t, bool> assignments = solver.solve(observed);
  boost::dynamic_bitset<> pred_input(64);

  // Fill with observed bits, if any input bits made it directly to output
  for (const auto &itr : observed) {
    const size_t bit_idx = itr.first;
    const bool bit_val = itr.second;
    if (bit_idx < 64) pred_input[bit_idx] = bit_val;
  }

  // Fill input prediction with assignments from the solver
  for (const auto &itr : assignments) {
    const size_t bit_idx = itr.first;
    const bool bit_val = itr.second;
    if (bit_idx < 64) pred_input[bit_idx] = bit_val;
  }

  assert(Utils::hexstr(pred_input).compare(Utils::hexstr(bits)) == 0);
  spdlog::info("CryptoMiniSAT tests passed.");
}

void bpTests() {
  /*
  A
   \
    XOR --> D
   /         \
  B           XOR --> F
   \         /
    AND --> E
   /
  C
                                             0    1    2    3    4    5
  F=1 --> E=1, D=0 --> B=1, C=1 --> A=1     (A=1, B=1, C=1, D=0, E=1, F=1)

  F=1 --> E=0, D=1 --> B=1 --> C=0, A=0     (A=0, B=1, C=0, D=1, E=0, F=1)
                       B=0 --> A=1, C=0     (A=1, B=0, C=0, D=1, E=0, F=1)
                                    C=1     (A=1, B=0, C=1, D=1, E=0, F=1)
  */

  std::map<size_t, bool> observed;
  observed[5] = true;
  // observed[3] = false;

  // Collect factors in a mapping from RV index --> factor
  std::map<size_t, Factor> factors;
  factors[0] = Factor(Factor::Type::PriorFactor, 0);
  factors[1] = Factor(Factor::Type::PriorFactor, 1);
  factors[2] = Factor(Factor::Type::PriorFactor, 2);
  factors[3] = Factor(Factor::Type::XorFactor, 3, {0, 1});
  factors[4] = Factor(Factor::Type::AndFactor, 4, {1, 2});
  factors[5] = Factor(Factor::Type::XorFactor, 5, {3, 4});

  bp::BPSolver solver(factors, {0, 1, 2});
  const std::map<size_t, bool> assignments = solver.solve(observed);
  for (const auto &itr : assignments) {
    const size_t rv = itr.first;
    const bool val = itr.second;
    spdlog::info("BP predicts bit {} = {}", rv, val);
  }
}

void allTests() {
  simpleTests();
  symBitVecTests();
  sha256Tests();
  ripemd160Tests();
  md5Tests();
  cmsatTests();
  // bpTests();
  // bitcoinBlockTest();
}

}  // end namespace preimage
