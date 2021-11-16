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

#include <unordered_map>

#include "core/solver.hpp"
#include "bp/graph.hpp"

namespace preimage {

namespace bp {

class BPSolver : public Solver {
 public:
  explicit BPSolver(bool verbose);

  std::string solverName() const override { return "Belief Propagation"; }

 protected:
  void initialize() override;

  std::unordered_map<int, bool> solveInternal() override;

 private:
  BPFactorType convertLogicGate(LogicGate::Type t) const;

  Graph g_;
};

}  // end namespace bp

}  // end namespace preimage
