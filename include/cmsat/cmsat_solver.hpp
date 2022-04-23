/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <cryptominisat5/cryptominisat.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "core/solver.hpp"

namespace preimage {

class CMSatSolver : public Solver {
 public:
  CMSatSolver();

  virtual ~CMSatSolver();

  std::string solverName() const override { return "CryptoMiniSAT"; }

 protected:
  void initialize() override;

  std::unordered_map<int, bool> solveInternal() override;

 private:
  inline CMSat::Lit getLit(int i) const {
    assert(i != 0);
    return CMSat::Lit(std::abs(i) - 1, i < 0);
  }

  void addClause(const LogicGate &g);

  void addXorClause(const LogicGate &g);

  CMSat::SATSolver *solver_;
};

}  // end namespace preimage
