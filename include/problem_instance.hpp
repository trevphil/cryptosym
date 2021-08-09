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
#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>
#include <assert.h>

#include "core/sym_hash.hpp"
#include "core/sym_bit_vec.hpp"
#include "core/solver.hpp"
#include "core/utils.hpp"
#include "hashing/hash_funcs.hpp"
#include "bp/bp_solver.hpp"
#include "cmsat/cmsat_solver.hpp"
#include "preimage_sat/preimage_sat.hpp"

#include <memory>
#include <string>

namespace preimage {

class ProblemInstance {
 public:
  ProblemInstance(int num_input_bits, int difficulty,
                  bool verbose, bool bin_format);

  int prepare(const std::string &hash_name,
              const std::string &solver_name);

  int execute();

 private:
  void saveSymbols(const std::string &filename);

  void loadSymbols(const std::string &filename);

  void createHasher(const std::string &hash_name);

  void createSolver(const std::string &solver_name);

  boost::dynamic_bitset<> getPreimage(const std::string &symbols_file,
                                      const std::string &hash_hex);

  std::unique_ptr<SymHash> hasher;
  std::unique_ptr<Solver> solver;

 private:
  int num_input_bits_;
  int difficulty_;
  bool verbose_;
  bool bin_format_;
};

}  // end namespace preimage
