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

#include <vector>

#include <eigen3/Eigen/Eigen>

#include "core/solver.hpp"

namespace preimage {

class SDPSolver : public Solver {
 public:
  explicit SDPSolver(bool verbose) : Solver(verbose) {}

  virtual ~SDPSolver() {
  }

  std::string solverName() const override { return "SDP"; }

 protected:
  void initialize() override {
    clauses_ = {};
    lit2clauses_ = {};

    for (const LogicGate &g : gates_) {
      const auto clauses = g.cnf();
      for (const std::vector<int> &clause : clauses) {
        clauses_.push_back(clause);
        const int clause_index = static_cast<int>(clauses_.size());
        for (int lit : clause) {
          const int signed_index = (lit < 0 ? -1 : 1) * clause_index;
          lit2clauses_[std::abs(lit)].push_back(signed_index);
        }
      }
    }

    n_ = num_vars_;
    m_ = static_cast<int>(clauses_.size());
    k_ = static_cast<int>(std::ceil(std::sqrt(2.0 * m_)));

    v_ = Eigen::MatrixXf::Random(k_, n_ + 1);
    const auto norm = v_.colwise().norm();
    v_.array().rowwise() /= norm.array();

    z_ = Eigen::MatrixXf::Zero(k_, m_);
    for (int j = 0; j < m_; ++j) {
      const std::vector<int> &clause = clauses_.at(j);
      for (int lit : clause) {
        const int8_t multiplier = lit < 0 ? -1 : 1;
        z_.col(j) += multiplier * v_.col(std::abs(lit));
      }
    }
  }

  std::unordered_map<int, bool> solveInternal() override {
    bool converged = false;
    while (!converged) {
      for (int i = 1; i <= n_; ++i) {
        const std::vector<int> &referenced_clauses = lit2clauses_[i];
        for (const int signed_index : referenced_clauses) {
          const int8_t s = signed_index < 0 ? -1 : 1;
          const int clause_index = std::abs(signed_index) - 1;
          z_.col(clause_index) -= s * v_.col(i);
        }

        v_.col(i).setZero();
      }
    }

    std::unordered_map<int, bool> solution;
    return solution;
  }

  std::vector<std::vector<int>> clauses_;
  std::unordered_map<int, std::vector<int>> lit2clauses_;
  int n_, m_, k_;
  Eigen::MatrixXf v_, z_;
};

}  // end namespace preimage
