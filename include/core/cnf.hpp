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

#include <set>
#include <unordered_map>
#include <vector>

#include "core/logic_gate.hpp"

namespace preimage {

class CNF {
 public:
  struct Simplification;

  CNF();

  explicit CNF(const std::vector<LogicGate> &gates);

  CNF(const std::vector<std::set<int>> &cls, int n_var);

  int numSatClauses(const std::unordered_map<int, bool> &assignments);

  double approximationRatio(const std::unordered_map<int, bool> &assignments);

  int num_vars;
  int num_clauses;
  std::vector<std::set<int>> clauses;
};

struct CNF::Simplification {
  Simplification();

  Simplification(const CNF &cnf, const std::unordered_map<int, bool> &assignments);

  CNF original_cnf, simplified_cnf;
  std::unordered_map<int, int> lit_simplified_to_original;
  std::unordered_map<int, bool> original_assignments;
};

}  // end namespace preimage
