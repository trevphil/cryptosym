/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <cryptominisat5/cryptominisat.h>

#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "core/logic_gate.hpp"
#include "core/solver.hpp"
#include "core/sym_representation.hpp"

namespace preimage {

class CMSatSolver : public Solver {
 public:
  CMSatSolver();

  virtual ~CMSatSolver();

  std::string solverName() const override { return "CryptoMiniSAT"; }

  std::unordered_map<int, bool> solve(
      const SymRepresentation &problem,
      const std::unordered_map<int, bool> &bit_assignments) override;

 private:
  void initializeSolver(int num_vars, const std::vector<LogicGate> &gates);

  inline CMSat::Lit getLit(int i) const {
    if (i == 0) {
      throw std::invalid_argument("Literals should be indexed starting at 1");
    }
    return CMSat::Lit(std::abs(i) - 1, i < 0);
  }

  void addClause(const LogicGate &g);

  void addXorClause(const LogicGate &g);

  CMSat::SATSolver *solver_;
};

}  // end namespace preimage
