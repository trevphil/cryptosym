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

#include <boost/dynamic_bitset.hpp>
#include <map>
#include <vector>
#include <assert.h>

#include "core/sym_hash.hpp"
#include "core/sym_bit_vec.hpp"
#include "core/solver.hpp"
#include "core/utils.hpp"
#include "hashing/hash_funcs.hpp"
#include "bp/bp_solver.hpp"
#include "cmsat/cmsat_solver.hpp"
#include "ortools_cp/cp_solver.hpp"

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

namespace preimage {

class ProblemInstance {
 public:
  ProblemInstance(size_t num_input_bits, int difficulty,
                  bool verbose, bool bin_format);

  int prepare(const std::string &hash_name,
              const std::string &solver_name);

  int execute();

 private:
  void prepareHasher(const std::string &hash_name);

  void prepareSolver(const std::string &solver_name);

  std::unique_ptr<SymHash> hasher;
  std::unique_ptr<Solver> solver;

 private:
  size_t num_input_bits_;
  int difficulty_;
  bool verbose_;
  bool bin_format_;
};

}  // end namespace preimage
