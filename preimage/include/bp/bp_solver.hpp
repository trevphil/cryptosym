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

#include <map>
#include <vector>

#include "core/solver.hpp"
#include "bp/graph.hpp"
#include "bp/node.hpp"
#include "bp/params.hpp"

namespace preimage {

namespace bp {

class BPSolver : public Solver {
 public:
  BPSolver(const std::map<size_t, Factor> &factors,
           const std::vector<size_t> &input_indices);

  std::string solverName() const override { return "Belief Propagation"; }

 protected:
  std::map<size_t, bool> solveInternal() override;

 private:
  BPFactorType convertFactorType(Factor::Type t) const;

  Graph g_;
};

}  // end namespace bp

}  // end namespace preimage
