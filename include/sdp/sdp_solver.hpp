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

#include <Eigen/Dense>
#include <Eigen/SVD>
#include <vector>

#include "core/cnf.hpp"
#include "core/solver.hpp"

namespace preimage {

class SDPSolver : public Solver {
 public:
  explicit SDPSolver(bool verbose) : Solver(verbose) {}

  virtual ~SDPSolver() {}

  std::string solverName() const override { return "SDP"; }

 protected:
  void initialize() override {
    simplification_ = CNF::Simplification(CNF(gates_), observed_);
    const CNF &cnf = simplification_.simplified_cnf;

    int clause_index = 1;
    lit2clauses_ = {};
    for (const std::set<int> &clause : cnf.clauses) {
      for (int lit : clause) {
        const int signed_index = (lit < 0 ? -1 : 1) * clause_index;
        lit2clauses_[std::abs(lit)].push_back(signed_index);
      }
      clause_index++;
    }

    n_ = cnf.num_vars;
    m_ = cnf.num_clauses;
    k_ = static_cast<int>(std::ceil(std::sqrt(2.0 * (n_ + 1))) + 1);
    if (verbose_) {
      spdlog::info("n={}, m={}, k={}, nk={}, mk={}", n_, m_, k_, n_ * k_, m_ * k_);
    }

    v_ = Eigen::MatrixXf::Random(k_, n_ + 1);
    const auto norm = v_.colwise().norm();
    v_.array().rowwise() /= norm.array();

    z_ = Eigen::MatrixXf::Zero(k_, m_);
    for (int j = 0; j < m_; ++j) {
      const std::set<int> &clause = cnf.clauses.at(j);
      for (int lit : clause) {
        const int8_t s = lit < 0 ? -1 : 1;
        z_.col(j) += s * v_.col(std::abs(lit));
      }
      z_.col(j) += (-1 * v_.col(0));
    }
  }

  float applyMixingKernel(const CNF &cnf) {
    float delta = 0.0;

    for (int i = 1; i <= n_; ++i) {
      const Eigen::VectorXf v_before = v_.col(i).replicate(1, 1);

      const std::vector<int> &referenced_clauses = lit2clauses_[i];
      for (const int signed_index : referenced_clauses) {
        const int8_t s = signed_index < 0 ? -1 : 1;
        const int j = std::abs(signed_index) - 1;
        z_.col(j) -= s * v_.col(i);
      }

      v_.col(i).setZero();
      for (const int signed_index : referenced_clauses) {
        const int8_t s = signed_index < 0 ? -1 : 1;
        const int j = std::abs(signed_index) - 1;
        const size_t nj = cnf.clauses.at(j).size();
        v_.col(i) -= (s / (4.0 * nj)) * z_.col(j);
      }
      const float v_norm = v_.col(i).norm();
      v_.col(i) /= v_norm;

      for (const int signed_index : referenced_clauses) {
        const int8_t s = signed_index < 0 ? -1 : 1;
        const int j = std::abs(signed_index) - 1;
        z_.col(j) += s * v_.col(i);
      }

      delta += v_norm * (v_before - v_.col(i)).squaredNorm();
    }

    return delta;
  }

  std::unordered_map<int, bool> solveInternal() override {
    int iter = 0;
    bool converged = false;
    float eps = 1e-5;
    const int random_rounding_trials = 2000;

    const CNF &cnf = simplification_.simplified_cnf;

    while (!converged) {
      const float delta = applyMixingKernel(cnf);
      if (iter > 0 && delta < eps) break;
      if (iter == 0) eps *= delta;
      ++iter;
      spdlog::info("Iteration {}, delta = {}\t(eps = {})", iter, delta, eps);
    }

    Eigen::MatrixXf r = Eigen::MatrixXf::Random(k_, random_rounding_trials);
    const auto norm = r.colwise().norm();
    r.array().rowwise() /= norm.array();

    auto sol = simplification_.original_assignments;
    // solveSVD(sol);
    // const double frac_sat = simplification_.original_cnf.approximationRatio(sol);
    // spdlog::info("Satisfied {} fraction of clauses", frac_sat);

    double best_fraction_sat = 0.0;
    for (int trial = 0; trial < random_rounding_trials; ++trial) {
      randomRouding(sol, r.col(trial));
      const double frac_sat = simplification_.original_cnf.approximationRatio(sol);
      if (frac_sat == 1.0) {
        spdlog::info("Recovered optimal solution at trial {}", trial);
        return sol;
      } else if (frac_sat > best_fraction_sat) {
        best_fraction_sat = frac_sat;
        spdlog::info("Satisfied {} fraction of clauses at trial {}", frac_sat, trial);
      }
    }

    return sol;
  }

  void solveSVD(std::unordered_map<int, bool> &sol) {
    // Note: M = (V * V^T) is symmetric, so (M + M^T) = 2 * M
    // const Eigen::MatrixXf A = (v_.rightCols(n_) * v_.rightCols(n_).transpose()) * 2;
    const Eigen::MatrixXf A = (v_ * v_.transpose()) * 2;

    // Note: in Eigen, singular values are always sorted in DECREASING order
    const auto svd = A.bdcSvd(Eigen::ComputeThinV);
    // Normal of separating hyperplane is Eigenvector for maximum Eigenvalue
    const Eigen::VectorXf normal = svd.matrixV().col(0);

    for (int i = 1; i <= n_; ++i) {
      const bool truth_val = v_.col(i).dot(normal) >= 0;
      const int literal = simplification_.lit_simplified_to_original[i];
      sol[literal] = truth_val;
      sol[-literal] = !truth_val;
    }
  }

  void randomRouding(std::unordered_map<int, bool> &sol, const Eigen::VectorXf &r) {
    const double v0_r = v_.col(0).dot(r);
    for (int i = 1; i <= n_; ++i) {
      const double dir = v_.col(i).dot(r);
      const bool truth_val = (v0_r * dir) >= 0;
      const int literal = simplification_.lit_simplified_to_original[i];
      sol[literal] = truth_val;
      sol[-literal] = !truth_val;
    }
  }

  CNF::Simplification simplification_;
  std::unordered_map<int, std::vector<int>> lit2clauses_;
  int n_, m_, k_;
  Eigen::MatrixXf v_, z_;
};

}  // end namespace preimage
