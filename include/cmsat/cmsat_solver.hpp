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
#include <set>
#include <vector>
#include <memory>

#include "core/solver.hpp"

namespace preimage {

class CMSatSolver : public Solver {
 public:
  explicit CMSatSolver(bool verbose);

  virtual ~CMSatSolver();

  std::string solverName() const override { return "CryptoMiniSAT"; }

 protected:
  void initialize() override;

  std::map<int, bool> solveInternal() override;

 private:
  CMSat::SATSolver *solver_;
  std::map<int, unsigned int> rv2idx_;
};

}  // end namespace preimage
