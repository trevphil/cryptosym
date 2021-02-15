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
#include <stdio.h>

#include "cmsat_solver.hpp"
#include "hash_funcs.hpp"
#include "sha256.hpp"
#include "sym_bit_vec.hpp"
#include "utils.hpp"
#include "factor.hpp"
#include "solver.hpp"

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

SymHash* selectHashFunction(const std::string &name) {
  if (name.compare("SHA256") == 0) {
    return new SHA256();
  } else if (name.compare("SameIOHash") == 0) {
    return new SameIOHash();
  } else if (name.compare("NotHash") == 0) {
    return new NotHash();
  } else if (name.compare("LossyPseudoHash") == 0) {
    return new LossyPseudoHash();
  } else if (name.compare("NonLossyPseudoHash") == 0) {
    return new NonLossyPseudoHash();
  } else {
    spdlog::error("Unrecognized hash function: {}", name);
    throw "Unrecognized hash function";
  }
}

Solver* selectSolver(const std::string &solving_method,
                     const std::map<size_t, Factor> &factors,
                     const std::vector<size_t> hash_input_indices) {
  if (solving_method.compare("cmsat") == 0) {
    return new CMSatSolver(factors, hash_input_indices);
  } else {
    spdlog::error("Unsupported solver: {}", solving_method);
    throw "Unsupported solver";
  }
}

std::string hash_func = "SHA256";
std::string solving_method = "cmsat";
size_t input_size = 64;
int difficulty = 1;

int parseArgument(char* arg) {
	int option;
	unsigned int uoption;
	char buf[1024];

  if (1 == sscanf(arg, "hash=%s", buf)) {
    hash_func = buf;
  } else if (1 == sscanf(arg, "d=%d", &option)) {
    difficulty = option;
  } else if (1 == sscanf(arg, "i=%u", &uoption)) {
    input_size = size_t(uoption);
  } else if (1 == sscanf(arg, "solver=%s", buf)) {
    solving_method = buf;
  } else {
    std::stringstream help_msg;
    help_msg << std::endl << "Command-line arguments:" << std::endl;
    help_msg << "\thash=HASH_FUNCTION" << std::endl;
    help_msg << "\t -> one of: SHA256, LossyPseudoHash, NonLossyPseudoHash, NotHash, SameIOHash" << std::endl;
    help_msg << "\td=DIFFICULTY (1-64)" << std::endl;
    help_msg << "\ti=NUM_INPUT_BITS (8-512 or more, best to choose a multiple of 8)" << std::endl;
    help_msg << "\tsolver=SOLVER (cmsat)" << std::endl;
    spdlog::info(help_msg.str());
    return 1;
  }

  return 0;
}

void run(int argc, char **argv) {
  // allTests();

  for (int i = 1; i < argc; i++) {
    if (parseArgument(argv[i])) return;
  }

  spdlog::info("Hash algorithm:\t{}", hash_func);
  spdlog::info("Solver:\t\t{}", solving_method);
  spdlog::info("Input message size:\t{} bits", input_size);
  spdlog::info("Difficulty level:\t{}", difficulty);

  // Execute hash algorithm on random input
  SymHash *h = selectHashFunction(hash_func);
  boost::dynamic_bitset<> input = Utils::randomBits(input_size, 0);
  SymBitVec output_bits = h->call(input, difficulty);
  const std::string output_hash = output_bits.hex();

  // Collect observed bits
  std::map<size_t, bool> observed;
  for (size_t i = 0; i < output_bits.size(); ++i) {
    const Bit &b = output_bits.at(i);
    if (b.is_rv) observed[b.index] = b.val;
  }

  // Collect factors in a mapping from RV index --> factor
  const size_t n = Factor::global_factors.size();
  std::map<size_t, Factor> factors;
  std::map<Factor::Type, size_t> factor_count;
  for (size_t i = 0; i < n; ++i) {
    const Factor &f = Factor::global_factors.at(i);
    assert(i == f.output);
    // Skip bits which are not ancestors of observed bits
    if (!h->canIgnore(i)) {
      factors[i] = f;
      factor_count[f.t]++;
    }
  }

  spdlog::info("Logic gate statistics:");
  spdlog::info(" --> # PRIOR: {}", factor_count[Factor::Type::PriorFactor]);
  spdlog::info(" --> # SAME: {}", factor_count[Factor::Type::SameFactor]);
  spdlog::info(" --> # NOT: {}", factor_count[Factor::Type::NotFactor]);
  spdlog::info(" --> # AND: {}", factor_count[Factor::Type::AndFactor]);
  spdlog::info(" --> # XOR: {}", factor_count[Factor::Type::XorFactor]);

  // Solve and extract the predicted input bits
  Solver *solver = selectSolver(solving_method, factors,
                                h->hashInputIndices());
  const std::map<size_t, bool> assignments = solver->solve(observed);
  boost::dynamic_bitset<> pred_input(input_size);

  // Fill with observed bits, if any input bits made it directly to output
  for (const auto &itr : observed) {
    const size_t bit_idx = itr.first;
    const bool bit_val = itr.second;
    if (bit_idx < input_size) pred_input[bit_idx] = bit_val;
  }

  // Fill input prediction with assignments from the solver
  for (const auto &itr : assignments) {
    const size_t bit_idx = itr.first;
    const bool bit_val = itr.second;
    if (bit_idx < input_size) pred_input[bit_idx] = bit_val;
  }

  spdlog::info("True input:\t\t{}", Utils::hexstr(input));
  spdlog::info("Reconstructed input:\t{}", Utils::hexstr(pred_input));

  // Check if prediction yields the same hash
  std::string pred_output = h->call(pred_input, difficulty).hex();
  if (output_hash.compare(pred_output) == 0) {
    spdlog::info("Success! Hashes match:\t{}", output_hash);
  } else {
    spdlog::warn("!!! Hashes do not match.");
    spdlog::warn("\tExpected:\t{}", output_hash);
    spdlog::warn("\tGot:\t\t{}", pred_output);
  }

  spdlog::info("Done.");

  // Free pointers and de-allocate memory
  delete h;
  h = NULL;
  delete solver;
  solver = NULL;
}

}  // end namespace preimage

int main(int argc, char **argv) {
  preimage::run(argc, argv);
  return 0;
}
