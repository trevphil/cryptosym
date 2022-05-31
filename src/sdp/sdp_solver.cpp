/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include "sdp/sdp_solver.hpp"

#include "core/config.hpp"

namespace preimage {

SDPSolver::SDPSolver(unsigned int rounding_trials)
    : Solver(), num_rounding_trials_(rounding_trials), n_(0), m_(0), k_(0) {}

SDPSolver::~SDPSolver() {}

void SDPSolver::initialize(const CNF &cnf) {
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
  k_ = static_cast<int>(std::ceil(std::sqrt(2.0 * n_))) + 1;
  if (config::verbose) {
    printf("%s: n=%d, m=%d, k=%d\n", solverName().c_str(), n_, m_, k_);
  }

  v_ = Eigen::MatrixXf::Random(n_ + 1, k_);  // Uniform (-1, 1)
  const auto norm = v_.rowwise().norm();
  v_.array().colwise() /= norm.array();

  z_ = Eigen::MatrixXf::Zero(m_, k_);
  for (int j = 0; j < m_; ++j) {
    const std::set<int> &clause = cnf.clauses.at(j);
    for (int lit : clause) {
      const float s = lit < 0 ? -1.0 : 1.0;
      z_.row(j) += s * v_.row(std::abs(lit));
    }
    z_.row(j) -= v_.row(0);
  }
}

float SDPSolver::applyMixingKernel(const CNF &cnf) {
  float delta = 0.0;

  for (int i = 1; i <= n_; ++i) {
    const Eigen::RowVectorXf v_before = v_.row(i).replicate(1, 1);

    const std::vector<int> &referenced_clauses = lit2clauses_[i];
    for (const int signed_index : referenced_clauses) {
      const float s = signed_index < 0 ? -1.0 : 1.0;
      const int j = std::abs(signed_index) - 1;
      z_.row(j) -= s * v_.row(i);
    }

    v_.row(i).setZero();
    for (const int signed_index : referenced_clauses) {
      const float s = signed_index < 0 ? -1.0 : 1.0;
      const int j = std::abs(signed_index) - 1;
      const size_t nj = cnf.clauses.at(j).size();
      v_.row(i) -= (s / (4.0 * nj)) * z_.row(j);
    }
    const float v_norm = v_.row(i).norm();
    v_.row(i) /= v_norm;

    for (const int signed_index : referenced_clauses) {
      const float s = signed_index < 0 ? -1.0 : 1.0;
      const int j = std::abs(signed_index) - 1;
      z_.row(j) += s * v_.row(i);
    }

    delta += v_norm * (v_before - v_.row(i)).squaredNorm();
  }

  return delta;
}

std::unordered_map<int, bool> SDPSolver::randomizedRoundingTrial(
    const Eigen::MatrixXf &v) const {
  Eigen::VectorXf r = Eigen::VectorXf::Random(v.cols());
  r.array() /= r.norm();
  const float s = r.dot(v.row(0));
  const Eigen::VectorXf same_sign = (v * r) * s;
  std::unordered_map<int, bool> assignments;
  assignments.reserve(n_);
  for (int i = 1; i <= n_; ++i) {
    assignments[i] = same_sign(i) >= 0;
  }
  return assignments;
}

std::unordered_map<int, bool> SDPSolver::solve(
    const SymRepresentation &problem,
    const std::unordered_map<int, bool> &bit_assignments) {
  std::unordered_map<int, bool> solution = bit_assignments;
  std::unordered_map<int, int> lit_new_to_old;
  const CNF cnf = problem.toCNF().simplify(solution, lit_new_to_old);

  if (cnf.num_vars == 0 || cnf.num_clauses == 0) {
    if (config::verbose) {
      printf("%s\n", "Problem solved directly from CNF simplification");
    }
    return solution;
  }

  initialize(cnf);

  int iter = 0;
  float eps = 1e-4;

  while (true) {
    const float delta = applyMixingKernel(cnf);
    if (iter > 0 && delta < eps) break;
    if (iter == 0) eps *= delta;
    ++iter;
    if (config::verbose) {
      printf("Iteration %d, delta = %.4f\t(eps = %.4f)\n", iter, delta, eps);
    }
  }

  double best_approx_ratio = 0.0;
  std::unordered_map<int, bool> best_assignment;

  for (unsigned int trial = 0; trial < num_rounding_trials_; ++trial) {
    const auto assignment = randomizedRoundingTrial(v_);
    const double approx_ratio = cnf.approximationRatio(assignment);
    if (approx_ratio > best_approx_ratio) {
      if (config::verbose) {
        printf("SDP satisfied %.4f fraction of clauses at trial %u\n", approx_ratio,
               trial);
      }
      best_approx_ratio = approx_ratio;
      best_assignment = assignment;
    }
    if (best_approx_ratio == 1.0) break;
  }

  for (const auto &itr : best_assignment) {
    const int lit = lit_new_to_old[itr.first];
    const bool truth_value = itr.second;
    solution[lit] = truth_value;
  }

  return solution;
}

}  // end namespace preimage
