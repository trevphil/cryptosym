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

#include <cryptominisat5/cryptominisat.h>

#include <map>
#include <vector>

#include "core/solver.hpp"

namespace preimage {

class CMSatSolver : public Solver {
 public:
  CMSatSolver(bool verbose);

  std::string solverName() const override { return "CryptoMiniSAT"; }

  void setUsableLogicGates() const override;

 protected:
  void initialize() override;

  std::map<size_t, bool> solveInternal() override;

 private:
  CMSat::SATSolver *solver_;
  std::map<size_t, unsigned int> rv2idx_;
};

}  // end namespace preimage
