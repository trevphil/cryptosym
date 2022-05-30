/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "core/bit_vec.hpp"
#include "core/logic_gate.hpp"
#include "core/sym_representation.hpp"

namespace preimage {

class Solver {
 public:
  Solver();

  virtual ~Solver();

  virtual std::unordered_map<int, bool> solve(const SymRepresentation &problem,
                                              const std::string &hash_hex);

  virtual std::unordered_map<int, bool> solve(const SymRepresentation &problem,
                                              const BitVec &hash_output);

  virtual std::unordered_map<int, bool> solve(
      const SymRepresentation &problem,
      const std::unordered_map<int, bool> &bit_assignments) = 0;

  virtual std::string solverName() const = 0;
};

}  // end namespace preimage
