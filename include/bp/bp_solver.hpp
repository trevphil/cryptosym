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

#include "bp/graph.hpp"
#include "core/bit_vec.hpp"
#include "core/logic_gate.hpp"
#include "core/solver.hpp"
#include "core/sym_representation.hpp"

namespace preimage {

namespace bp {

class BPSolver : public Solver {
 public:
  BPSolver();

  std::string solverName() const override { return "Belief Propagation"; }

  std::unordered_map<int, bool> solve(const SymRepresentation &problem,
                                      const std::string &hash_hex) override {
    return Solver::solve(problem, hash_hex);
  }

  std::unordered_map<int, bool> solve(const SymRepresentation &problem,
                                      const BitVec &hash_output) override {
    return Solver::solve(problem, hash_output);
  }

  std::unordered_map<int, bool> solve(
      const SymRepresentation &problem,
      const std::unordered_map<int, bool> &bit_assignments) override;

 private:
  void initializeGraph(const std::vector<LogicGate> &gates);

  BPFactorType convertLogicGate(LogicGate::Type t) const;

  Graph g_;
};

}  // end namespace bp

}  // end namespace preimage
