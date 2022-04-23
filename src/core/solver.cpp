/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include "core/solver.hpp"

#include <algorithm>

#include "core/config.hpp"
#include "core/utils.hpp"

namespace preimage {

Solver::Solver() {}

Solver::~Solver() {}

void Solver::setHeader(int inp_size, int out_size, int n_vars, int n_gates) {
  input_size_ = inp_size;
  output_size_ = out_size;
  num_vars_ = n_vars;
  num_gates_ = n_gates;
}

void Solver::setGates(const std::vector<LogicGate> &gates) { gates_ = gates; }

void Solver::setInputIndices(const std::vector<int> &input_indices) {
  input_indices_ = input_indices;
}

void Solver::setOutputIndices(const std::vector<int> &output_indices) {
  output_indices_ = output_indices;
}

void Solver::setObserved(const std::unordered_map<int, bool> &observed) {
  observed_ = observed;
}

std::unordered_map<int, bool> Solver::solve() {
  if (num_gates_ != (int)gates_.size()) {
    printf("Expected %d gates but have %lu.\n", num_gates_, gates_.size());
    assert(false);
  }
  if (input_size_ != (int)input_indices_.size()) {
    printf("Expected input_size=%d but it is %lu.\n", input_size_, input_indices_.size());
    assert(false);
  }
  if (output_size_ != (int)output_indices_.size()) {
    printf("Expected output_size=%d but it is %lu.\n", output_size_,
           output_indices_.size());
    assert(false);
  }

  const auto start = Utils::ms_since_epoch();
  initialize();
  auto solution = solveInternal();
  const auto end = Utils::ms_since_epoch();
  if (config::verbose) printf("Solver finished in %lld ms\n", end - start);

  // Fill in observed values
  for (const auto &itr : observed_) {
    assert(itr.first > 0);
    if (solution.count(itr.first) > 0) {
      // Check for solver predictions which conflict with observations
      if (solution.at(itr.first) != itr.second) {
        printf("Variable %d is %d but was predicted %d!\n", itr.first, itr.second,
               solution.at(itr.first));
        assert(false);
      }
    } else {
      solution[itr.first] = itr.second;
    }
  }

  return solution;
}

}  // end namespace preimage