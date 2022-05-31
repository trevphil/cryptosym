/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#pragma once

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/logic_gate.hpp"

namespace preimage {

class CNF {
 public:
  CNF();

  explicit CNF(const std::vector<LogicGate> &gates);

  CNF(const std::vector<std::set<int>> &cls, int n_var);

  int numSatClauses(const std::unordered_map<int, bool> &assignments) const;

  double approximationRatio(const std::unordered_map<int, bool> &assignments) const;

  void toFile(const std::string &filename) const;

  static CNF fromFile(const std::string &filename);

  void toMIS(const std::string &filename) const;

  void toGraphColoring(const std::string &filename) const;

  CNF simplify(std::unordered_map<int, bool> &assignments,
               std::unordered_map<int, int> &lit_new_to_old) const;

  int num_vars;
  int num_clauses;
  std::vector<std::set<int>> clauses;
};

}  // end namespace preimage
