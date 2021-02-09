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

#include "solver.hpp"

#include <spdlog/spdlog.h>

namespace dataset_generator {

Solver::Solver(const std::vector<Factor> &factors,
               const std::vector<size_t> &input_indices)
    : factors_(factors), input_indices_(input_indices) {}

Solver::~Solver() {}

std::map<size_t, bool> Solver::solve(const std::map<size_t, bool> &observed) {
  reset();
  observed_ = observed;
  spdlog::error("Calling solve() from generic superclass");
  std::map<size_t, bool> solution;
  return solution;
}

void Solver::reset() { observed_.clear(); }

void Solver::setImplicitObserved() {}

void Solver::propagateBackward() {}

void Solver::propagateForward() {}

}  // end namespace dataset_generator