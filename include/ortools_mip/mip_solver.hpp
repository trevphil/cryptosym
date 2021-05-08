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

#include "ortools/linear_solver/linear_solver.h"

#include <map>
#include <vector>
#include <memory>
#include <assert.h>

#include "core/solver.hpp"

namespace preimage {

class ORToolsMIPSolver : public Solver {
 public:
  ORToolsMIPSolver(bool verbose);

  std::string solverName() const override { return "ortools MIP"; }

  void setUsableLogicGates() const override;

 protected:
  inline std::string rv2s(size_t rv) const;

  inline operations_research::MPVariable* getVar(size_t rv);

  void initialize() override;

  std::map<size_t, bool> solveInternal() override;

 private:
  std::unique_ptr<operations_research::MPSolver> solver_;
  std::map<size_t, operations_research::MPVariable*> rv2var_;
};

}  // end namespace preimage
