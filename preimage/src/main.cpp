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
#include <vector>
#include <string>
#include <stdio.h>

#include "tests.hpp"
#include "core/sym_bit_vec.hpp"
#include "core/utils.hpp"
#include "core/factor.hpp"
#include "core/solver.hpp"
#include "hashing/hash_funcs.hpp"
#include "cmsat/cmsat_solver.hpp"
#include "bp/bp_solver.hpp"

namespace preimage {

SymHash* selectHashFunction(const std::string &name) {
  if (name.compare("SHA256") == 0) {
    return new SHA256();
  } else if (name.compare("MD5") == 0) {
    return new MD5();
  } else if (name.compare("RIPEMD160") == 0) {
    return new RIPEMD160();
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
  } else if (solving_method.compare("bp") == 0) {
    return new bp::BPSolver(factors, hash_input_indices);
  } else {
    spdlog::error("Unsupported solver: {}", solving_method);
    throw "Unsupported solver";
  }
}

std::string hash_func = "MD5";
std::string solving_method = "bp";
size_t input_size = 64;
int difficulty = -1;
int run_tests = false;

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
  } else if (1 == sscanf(arg, "tests=%d", &option)) {
    run_tests = (option == 1);
  } else {
    std::stringstream help_msg;
    help_msg << std::endl << "Command-line arguments:" << std::endl;
    help_msg << "\thash=HASH_FUNCTION" << std::endl;
    help_msg << "\t -> one of: SHA256, MD5, RIPEMD160, LossyPseudoHash, NonLossyPseudoHash, NotHash, SameIOHash" << std::endl;
    help_msg << "\td=DIFFICULTY (-1 for default)" << std::endl;
    help_msg << "\ti=NUM_INPUT_BITS (choose a multiple of 8)" << std::endl;
    help_msg << "\tsolver=SOLVER" << std::endl;
    help_msg << "\t -> one of: cmsat, bp" << std::endl;
    help_msg << "\ttests=1 (if you want to run tests)" << std::endl;
    spdlog::info(help_msg.str());
    return 1;
  }

  return 0;
}

void run(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (parseArgument(argv[i])) return;
  }

  if (run_tests) {
    spdlog::info("Running tests...");
    allTests();
    spdlog::info("All tests finished.");
    return;
  }

  SymHash *h = selectHashFunction(hash_func);
  if (difficulty == -1) difficulty = h->defaultDifficulty();

  // Execute hash algorithm on random input
  boost::dynamic_bitset<> input = Utils::randomBits(input_size, 0);
  SymBitVec output_bits = h->call(input, difficulty);
  const std::string output_hash = output_bits.bin();

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

  // Initialize the solver
  Solver *solver = selectSolver(solving_method, factors,
                                h->hashInputIndices());

  spdlog::info("Hash algorithm:\t{}", h->hashName());
  spdlog::info("Solver:\t\t{}", solver->solverName());
  spdlog::info("Input message size:\t{} bits", input_size);
  spdlog::info("Difficulty level:\t{}", difficulty);

  spdlog::info("Logic gate statistics:");
  spdlog::info(" --> # PRIOR: {}", factor_count[Factor::Type::PriorFactor]);
  spdlog::info(" --> # SAME: {}", factor_count[Factor::Type::SameFactor]);
  spdlog::info(" --> # NOT: {}", factor_count[Factor::Type::NotFactor]);
  spdlog::info(" --> # AND: {}", factor_count[Factor::Type::AndFactor]);
  spdlog::info(" --> # XOR: {}", factor_count[Factor::Type::XorFactor]);
  spdlog::info(" --> # OR: {}", factor_count[Factor::Type::OrFactor]);

  // Solve and extract the predicted input bits
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

  spdlog::info("True input:\t\t{}", Utils::binstr(input));
  spdlog::info("Reconstructed input:\t{}", Utils::binstr(pred_input));

  // Check if prediction yields the same hash
  std::string pred_output = h->call(pred_input, difficulty).bin();
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
