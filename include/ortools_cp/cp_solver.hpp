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

#include "ortools/sat/cp_model.h"
#include "ortools/sat/model.h"
#include "ortools/sat/sat_parameters.pb.h"

#include <map>
#include <vector>
#include <assert.h>

#include "core/solver.hpp"

namespace preimage {

class ORToolsCPSolver : public Solver {
 public:
  ORToolsCPSolver(bool verbose);

  std::string solverName() const override { return "ortools CP"; }

  void setUsableLogicGates() const override;

 protected:
  inline std::string rv2s(size_t rv) const;

  inline operations_research::sat::BoolVar getVar(size_t rv);

  void initialize() override;

  std::map<size_t, bool> solveInternal() override;

 private:
  operations_research::sat::CpModelBuilder cp_model_;
  std::map<size_t, operations_research::sat::BoolVar> rv2var_;
};

}  // end namespace preimage
