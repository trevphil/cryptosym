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
#include <map>
#include <string>

#include "cmsat_solver.hpp"
#include "hash_funcs.hpp"
#include "sha256.hpp"
#include "sym_bit_vec.hpp"
#include "utils.hpp"

namespace dataset_generator {

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

  spdlog::info("SymBitVec tests passed");
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
  const int difficulty = 64;  // Normal SHA256 has 64 rounds

  std::string h = sha256.call(Utils::str2bits(empty), difficulty).hex();
  // spdlog::info("empty result:\nGot\t{}\nExpect\t{}", h, h_empty);
  assert(h.compare(h_empty) == 0);

  h = sha256.call(Utils::str2bits(s), difficulty).hex();
  // spdlog::info("s result:\nGot\t{}\nExpect\t{}", h, h_s);
  assert(h.compare(h_s) == 0);

  h = sha256.call(Utils::str2bits(s7), difficulty).hex();
  // spdlog::info("s7 result:\nGot\t{}\nExpect\t{}", h, h_s7);
  assert(h.compare(h_s7) == 0);

  spdlog::info("SHA256 tests passed.");
}

void bitcoinGenesisBlockTest() {
  // TODO: fix genesis block
  const std::vector<bool> genesis_block = {
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1,
      0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0,
      0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0,
      1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1,
      1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1,
      1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0,
      1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0,
      1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1,
      0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1,
      1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0,
      1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0,
      1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1,
      0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1,
      0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1,
      1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1,
      0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0,
      1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0,
      1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1,
      0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 0, 1};
  boost::dynamic_bitset bits(genesis_block.size());
  for (size_t i = 0; i < bits.size(); ++i) bits[i] = genesis_block.at(i);

  SHA256 sha256;
  const SymBitVec h1 = sha256.call(bits, 64);
  const std::string h2 = sha256.call(h1.bits(), 64).hex();
  spdlog::info(h2);
}

void allTests() {
  simpleTests();
  symBitVecTests();
  sha256Tests();
  // bitcoinGenesisBlockTest();
}

void run(int argc, char **argv) {
  // allTests();

  const size_t input_size = 64;
  const int difficulty = 17;

  spdlog::info("Hash algorithm:\t{}", "SHA256");
  spdlog::info("Input message size:\t{} bits", input_size);
  spdlog::info("Difficulty level:\t{}", difficulty);

  // Execute hash algorithm on random input
  SHA256 sha256;
  boost::dynamic_bitset<> input = Utils::randomBits(input_size, 0);
  SymBitVec output_bits = sha256.call(input, difficulty);
  const std::string output_hash = output_bits.hex();

  // Collect observed bits
  std::map<size_t, bool> observed;
  for (size_t i = 0; i < output_bits.size(); ++i) {
    const Bit &b = output_bits.at(i);
    observed[b.index] = b.val;
  }

  // Collect factors
  const std::vector<Factor> &factors = Factor::global_factors;
  const size_t n = factors.size();
  for (size_t i = 0; i < n; ++i) assert(i == factors.at(i).output);

  // Solve and extract the predicted input bits
  CMSatSolver solver(factors, sha256.hashInputIndices());
  const std::map<size_t, bool> assignments = solver.solve(observed);
  boost::dynamic_bitset<> pred_input(input_size);
  for (const auto &itr : assignments) {
    const size_t bit_idx = itr.first;
    const bool bit_val = itr.second;
    if (bit_idx < input_size) pred_input[bit_idx] = bit_val;
  }

  spdlog::info("True input:\t\t{}", Utils::hexstr(input));
  spdlog::info("Reconstructed input:\t{}", Utils::hexstr(pred_input));

  // Check if prediction yields the same hash
  std::string pred_output = sha256.call(pred_input, difficulty).hex();
  if (output_hash.compare(pred_output) == 0) {
    spdlog::info("Success! Hashes match:\t{}", output_hash);
  } else {
    spdlog::warn("!!! Hashes do not match.");
    spdlog::warn("\tExpected:\t{}", output_hash);
    spdlog::warn("\tGot:\t\t{}", pred_output);
  }

  spdlog::info("Done.");
}

}  // end namespace dataset_generator

int main(int argc, char **argv) {
  dataset_generator::run(argc, argv);
  return 0;
}
