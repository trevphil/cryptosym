/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#pragma once

#include <Eigen/Dense>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/cnf.hpp"
#include "core/solver.hpp"
#include "core/sym_representation.hpp"

namespace preimage {

class SDPSolver : public Solver {
 public:
  SDPSolver(unsigned int rounding_trials = 100);

  virtual ~SDPSolver();

  std::string solverName() const override { return "SDP Mixing Method"; }

  std::unordered_map<int, bool> solve(const SymRepresentation &problem,
                                      const std::string &hash_hex) override {
    return Solver::solve(problem, hash_hex);
  }

  std::unordered_map<int, bool> solve(const SymRepresentation &problem,
                                      const BitVec &hash_output) override {
    return Solver::solve(problem, hash_output);
  }

  Eigen::MatrixXf mixingMethod(const CNF &cnf);

  std::unordered_map<int, bool> randomizedRoundingTrial(const Eigen::MatrixXf &v) const;

  std::unordered_map<int, bool> solve(
      const SymRepresentation &problem,
      const std::unordered_map<int, bool> &bit_assignments) override;

 protected:
  void initialize(const CNF &cnf);

  float applyMixingKernel();

  unsigned int num_rounding_trials_;
  int n_, m_, k_;
  std::unordered_map<int, std::vector<int>> lit2clauses_;
  Eigen::MatrixXf v_, z_;
  Eigen::VectorXf loss_weights_;
};

}  // end namespace preimage
