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
#include "core/solver.hpp"
#include "core/sym_representation.hpp"

namespace preimage {

class DAGSolver : public Solver {
 public:
  struct LitStats {
    LitStats() {}
    LitStats(int i, bool a) : lit(i), preferred_assignment(a) {}

    int lit;
    bool preferred_assignment;
    int num_referenced_gates;

    double score() const { return static_cast<double>(num_referenced_gates); }
  };

  struct StackItem {
    StackItem() {}
    StackItem(int lit) : lit_guess(lit), implied({}), second_try(false) {}
    int lit_guess;
    std::set<int> implied;
    bool second_try;
  };

  DAGSolver();

  virtual ~DAGSolver();

  std::string solverName() const override { return "DAG Solver"; }

  std::unordered_map<int, bool> solve(
      const SymRepresentation &problem,
      const std::unordered_map<int, bool> &bit_assignments) override;

 private:
  void initialize(const std::vector<LogicGate> &gates);

  LitStats computeStats(const int lit);

  void pushStack(int lit, bool truth_value, bool second_try);

  void popStack(int &lit, bool &truth_value);

  void popStack();

  int pickLiteral(bool &assignment);

  int propagate(const int lit, const std::vector<LogicGate> &gates);

  inline bool getLitValue(const int lit) const {
    return lit < 0 ? (literals[-lit] < 0) : (literals[lit] > 0);
  }

  inline void setLitValue(const int lit, const bool val) {
    if (lit < 0) {
      literals[-lit] = val ? -1 : 1;
    } else {
      literals[lit] = val ? 1 : -1;
    }
  }

  bool partialSolve(const LogicGate &g, std::vector<int> &solved_lits);

  bool partialSolveAnd(const LogicGate &g, std::vector<int> &solved_lits);

  bool partialSolveOr(const LogicGate &g, std::vector<int> &solved_lits);

  bool partialSolveXor(const LogicGate &g, std::vector<int> &solved_lits);

  bool partialSolveXor3(const LogicGate &g, std::vector<int> &solved_lits);

  bool partialSolveMaj(const LogicGate &g, std::vector<int> &solved_lits);

  int num_vars_;
  std::vector<int8_t> literals;
  std::vector<StackItem> stack;
  std::vector<LitStats> literal_ordering;
  std::unordered_map<int, std::set<int>> lit2gates;
};

}  // end namespace preimage
