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

#include "core/config.hpp"
#include "core/logic_gate.hpp"
#include "core/sym_bit_vec.hpp"
#include "core/utils.hpp"
#include "hashing/hash_funcs.hpp"
#include "problem_instance.hpp"

namespace preimage {

void conversionTests() {
  const boost::dynamic_bitset<> bits(16, 0b1101001100011101);
  const SymBitVec bv(bits);
  const std::string hex = "d31d";
  const std::string bin = "1101001100011101";
  assert(bv.intVal() == 0b1101001100011101);
  assert(bv.bin(false).compare(bin) == 0);
  assert(bv.hex().compare(hex) == 0);
  assert(Utils::hexstr(bits).compare(hex) == 0);
  assert(Utils::binstr(bits).compare(bin) == 0);
  assert(Utils::hex2bits(hex) == bits);
  spdlog::info("Conversion tests passed.");
}

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

  std::string h = Utils::hexstr(sha256.call(Utils::str2bits(empty)));
  assert(h.compare(h_empty) == 0);

  h = Utils::hexstr(sha256.call(Utils::str2bits(s)));
  assert(h.compare(h_s) == 0);

  h = Utils::hexstr(sha256.call(Utils::str2bits(s7)));
  assert(h.compare(h_s7) == 0);

  Utils::seed(1);
  const std::vector<int> inp_sizes{0, 8, 32, 64, 512, 640, 1024};
  for (int inp_size : inp_sizes) {
    for (int sample_idx = 0; sample_idx < 10; sample_idx++) {
      // Generate a random input of length "inp_size"
      const boost::dynamic_bitset<> bits = Utils::randomBits(inp_size);
      const int n_bytes = inp_size / 8;
      uint8_t byte_arr[n_bytes];
      for (int byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
        byte_arr[byte_idx] = 0;
        for (int k = 0; k < 8; k++) {
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
      h = Utils::hexstr(sha256.call(bits));

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
  const std::vector<int> inp_sizes{0, 8, 32, 64, 512, 640, 1024};
  for (int inp_size : inp_sizes) {
    for (int sample_idx = 0; sample_idx < 10; sample_idx++) {
      // Generate a random input of length "inp_size" bits
      const boost::dynamic_bitset<> bits = Utils::randomBits(inp_size);
      const int n_bytes = inp_size / 8;
      uint8_t byte_arr[n_bytes];
      for (int byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
        byte_arr[byte_idx] = 0;
        for (int k = 0; k < 8; k++) {
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
      const std::string h = Utils::hexstr(ripemd160.call(bits));

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
  const std::vector<int> inp_sizes{0, 8, 32, 64, 512, 640, 1024};
  for (int inp_size : inp_sizes) {
    for (int sample_idx = 0; sample_idx < 10; sample_idx++) {
      // Generate a random input of length "inp_size" bits
      const boost::dynamic_bitset<> bits = Utils::randomBits(inp_size);
      const int n_bytes = inp_size / 8;
      uint8_t byte_arr[n_bytes];
      for (int byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
        byte_arr[byte_idx] = 0;
        for (int k = 0; k < 8; k++) {
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
      const std::string h = Utils::hexstr(md5.call(bits));

      if (h.compare(expected_output) != 0) {
        spdlog::info("inp_size={}, sample={}\n\tInput:\t{}\n\tExpected:\t{}\n\tGot:\t\t{}",
                     inp_size, sample_idx, Utils::hexstr(bits), expected_output, h);
        assert(false);
      }
    }
  }

  spdlog::info("MD5 tests passed, mean runtime: ~{:.0f} ms", md5.averageRuntimeMs());
}

void cmsatTests() {
  ProblemInstance problem(64, 12, false, false);

  int rtn = problem.prepare("SHA256", "cmsat");
  assert(rtn == 0);
  int status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("MD5", "cmsat");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("RIPEMD160", "cmsat");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  spdlog::info("CryptoMiniSAT tests passed.");
}

void preimageSATTests() {
  ProblemInstance problem(64, 12, false, false);

  int rtn = problem.prepare("SHA256", "simple");
  assert(rtn == 0);
  int status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("MD5", "simple");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("RIPEMD160", "simple");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  spdlog::info("PreimageSAT tests passed.");
}

void bpTests() {
  ProblemInstance problem(64, 2, false, false);

  int rtn = problem.prepare("SHA256", "bp");
  assert(rtn == 0);
  int status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("MD5", "bp");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("RIPEMD160", "bp");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  spdlog::info("Belief propagation tests passed.");
}

void allTests() {
  spdlog::info("Doing simple tests..");
  conversionTests();
  simpleTests();
  symBitVecTests();
  config::only_and_gates = true;
  spdlog::info("Validating hash functions using only AND gates...");
  sha256Tests();
  ripemd160Tests();
  md5Tests();
  config::only_and_gates = false;
  spdlog::info("Validating hash functions using all gates...");
  sha256Tests();
  ripemd160Tests();
  md5Tests();
  spdlog::info("Validating solvers...");
  cmsatTests();
  preimageSATTests();
  bpTests();
}

}  // end namespace preimage

int main(int argc, char **argv) {
  spdlog::info("Running tests...");
  preimage::allTests();
  spdlog::info("All tests finished.");
  return 0;
}
