/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <unordered_map>

#include "bp/graph.hpp"
#include "core/logic_gate.hpp"
#include "core/solver.hpp"
#include "core/sym_representation.hpp"

namespace preimage {

namespace bp {

class BPSolver : public Solver {
 public:
  BPSolver();

  std::string solverName() const override { return "Belief Propagation"; }

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
