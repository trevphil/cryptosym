/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <boost/dynamic_bitset.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include "core/solver.hpp"
#include "core/sym_hash.hpp"

namespace preimage {

class ProblemInstance {
 public:
  ProblemInstance(int num_input_bits, int difficulty, bool bin_format);

  int prepare(const std::string &hash_name, const std::string &solver_name);

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
  bool bin_format_;
};

}  // end namespace preimage
