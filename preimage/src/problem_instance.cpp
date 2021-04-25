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

#include "problem_instance.hpp"

namespace preimage {

ProblemInstance::ProblemInstance(size_t num_input_bits, int difficulty,
                                 bool verbose, bool bin_format)
    : num_input_bits_(num_input_bits), difficulty_(difficulty),
      verbose_(verbose), bin_format_(bin_format) {}

int ProblemInstance::prepare(const std::string &hash_name,
                             const std::string &solver_name) {
  prepareHasher(hash_name);
  if (hasher == nullptr) return 1;
  prepareSolver(solver_name);
  if (solver == nullptr) return 1;
  solver->setUsableLogicGates();
  return 0;
}

int ProblemInstance::execute() {
  if (hasher == nullptr || solver == nullptr) return 1;

  if (difficulty_ == -1) {
    difficulty_ = hasher->defaultDifficulty();
  }

  if (verbose_) {
    spdlog::info("Executing problem instance...");
    spdlog::info("Hash algorithm:\t{}", hasher->hashName());
    spdlog::info("Solver:\t\t{}", solver->solverName());
    spdlog::info("Input message size:\t{} bits", num_input_bits_);
    spdlog::info("Difficulty level:\t{}", difficulty_);
  }

  // Execute hash algorithm on random input
  boost::dynamic_bitset<> input = Utils::randomBits(num_input_bits_, 0);
  SymBitVec output_bits = hasher->call(input, difficulty_);
  const std::string output_hash =
      bin_format_ ? output_bits.bin() : output_bits.hex();

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
    if (!hasher->canIgnore(i)) {
      factors[i] = f;
      factor_count[f.t]++;
    }
  }

  if (verbose_) {
    spdlog::info("Logic gate statistics:");
    spdlog::info(" --> # PRIOR: {}", factor_count[Factor::Type::PriorFactor]);
    spdlog::info(" --> # SAME: {}", factor_count[Factor::Type::SameFactor]);
    spdlog::info(" --> # NOT: {}", factor_count[Factor::Type::NotFactor]);
    spdlog::info(" --> # AND: {}", factor_count[Factor::Type::AndFactor]);
    spdlog::info(" --> # XOR: {}", factor_count[Factor::Type::XorFactor]);
    spdlog::info(" --> # OR: {}", factor_count[Factor::Type::OrFactor]);
  }

  // Solve and extract the predicted input bits
  const std::map<size_t, bool> assignments =
      solver->solve(factors, hasher->hashInputIndices(), observed);
  boost::dynamic_bitset<> pred_input(num_input_bits_);

  // Fill with observed bits, if any input bits made it directly to output
  for (const auto &itr : observed) {
    const size_t bit_idx = itr.first;
    const bool bit_val = itr.second;
    if (bit_idx < num_input_bits_) pred_input[bit_idx] = bit_val;
  }

  // Fill input prediction with assignments from the solver
  for (const auto &itr : assignments) {
    const size_t bit_idx = itr.first;
    const bool bit_val = itr.second;
    if (bit_idx < num_input_bits_) pred_input[bit_idx] = bit_val;
  }

  if (verbose_) {
    if (bin_format_) {
      spdlog::info("True input:\t\t{}", Utils::binstr(input));
      spdlog::info("Reconstructed input:\t{}", Utils::binstr(pred_input));
    } else {
      spdlog::info("True input:\t\t{}", Utils::hexstr(input));
      spdlog::info("Reconstructed input:\t{}", Utils::hexstr(pred_input));
    }
  }

  // Check if prediction yields the same hash
  const SymBitVec pred_output_bits = hasher->call(pred_input, difficulty_);
  const std::string pred_output =
      bin_format_ ? pred_output_bits.bin() : pred_output_bits.hex();
  if (output_hash.compare(pred_output) == 0) {
    if (verbose_) spdlog::info("Success! Hashes match:\t{}", output_hash);
    return 0;
  } else {
    if (verbose_) {
      spdlog::warn("!!! Hashes do not match.");
      spdlog::warn("\tExpected:\t{}", output_hash);
      spdlog::warn("\tGot:\t\t{}", pred_output);
    }
    return 1;
  }
}

void ProblemInstance::prepareHasher(const std::string &hash_name) {
  if (hash_name.compare("SHA256") == 0) {
    hasher = std::unique_ptr<SymHash>(new SHA256());
  } else if (hash_name.compare("MD5") == 0) {
    hasher = std::unique_ptr<SymHash>(new MD5());
  } else if (hash_name.compare("RIPEMD160") == 0) {
    hasher = std::unique_ptr<SymHash>(new RIPEMD160());
  } else if (hash_name.compare("SameIOHash") == 0) {
    hasher = std::unique_ptr<SymHash>(new SameIOHash());
  } else if (hash_name.compare("NotHash") == 0) {
    hasher = std::unique_ptr<SymHash>(new NotHash());
  } else if (hash_name.compare("LossyPseudoHash") == 0) {
    hasher = std::unique_ptr<SymHash>(new LossyPseudoHash());
  } else if (hash_name.compare("NonLossyPseudoHash") == 0) {
    hasher = std::unique_ptr<SymHash>(new NonLossyPseudoHash());
  } else {
    spdlog::error("Unrecognized hash function: {}", hash_name);
    hasher = nullptr;
  }
}

void ProblemInstance::prepareSolver(const std::string &solver_name) {
  if (solver_name.compare("cmsat") == 0) {
    solver = std::unique_ptr<Solver>(new CMSatSolver(verbose_));
  } else if (solver_name.compare("bp") == 0) {
    solver = std::unique_ptr<Solver>(new bp::BPSolver(verbose_));
  } else if (solver_name.compare("ortools_cp") == 0) {
    solver = std::unique_ptr<Solver>(new ORToolsCPSolver(verbose_));
  } else if (solver_name.compare("ortools_mip") == 0) {
    solver = std::unique_ptr<Solver>(new ORToolsMIPSolver(verbose_));
  } else {
    spdlog::error("Unsupported solver: {}", solver_name);
    solver = nullptr;
  }
}

}  // end namespace preimage
