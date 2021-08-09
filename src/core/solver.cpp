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

#include <list>
#include <algorithm>

#include "core/solver.hpp"
#include "core/utils.hpp"

#include <spdlog/spdlog.h>

namespace preimage {

Solver::Solver(bool verbose) : verbose_(verbose) {}

Solver::~Solver() {}

void Solver::setHeader(int inp_size, int out_size, int n_vars, int n_gates) {
  input_size_ = inp_size;
  output_size_ = out_size;
  num_vars_ = n_vars;
  num_gates_ = n_gates;
}

void Solver::setGates(const std::vector<LogicGate> &gates) {
  gates_ = gates;
}

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
    spdlog::error("Expected {} gates but have {}.", num_gates_, gates_.size());
    assert(false);
  }
  if (input_size_ != (int)input_indices_.size()) {
    spdlog::error("Expected input_size={} but it is {}.",
                  input_size_, input_indices_.size());
    assert(false);
  }
  if (output_size_ != (int)output_indices_.size()) {
    spdlog::error("Expected output_size={} but it is {}.",
                  output_size_, output_indices_.size());
    assert(false);
  }

  const auto start = Utils::ms_since_epoch();
  initialize();
  auto solution = solveInternal();
  const auto end = Utils::ms_since_epoch();
  if (verbose_) spdlog::info("Solver finished in {} ms", end - start);

  // Fill in observed values
  for (const auto &itr : observed_) {
    assert(itr.first > 0);
    if (solution.count(itr.first) > 0) {
      // Check for solver predictions which conflict with observations
      if (solution.at(itr.first) != itr.second) {
        spdlog::error("Variable {} is {} but was predicted {}!",
                      itr.first, itr.second, solution.at(itr.first));
        assert(false);
      }
    } else {
      solution[itr.first] = itr.second;
    }
  }

  return solution;
}

int Solver::writeCNF(const std::string &filename) const {
  int num_clauses = static_cast<int>(observed_.size());
  for (const LogicGate &g : gates_) {
    num_clauses += LogicGate::numClausesCNF(g.t());
  }

  std::ofstream cnf_file(filename);
  if (!cnf_file.is_open()) {
    spdlog::error("Unable to open \"{}\" in write mode.", filename);
    assert(false);
    return 0;
  }

  cnf_file << "p cnf " << num_vars_ << " " << num_clauses << "\n";
  for (const auto &itr : observed_) {
    assert(itr.first > 0);
    cnf_file << (itr.first * (itr.second ? 1 : -1)) << " 0\n";
  }
  for (const LogicGate &g : gates_) {
    const std::vector<std::vector<int>> clauses = g.cnf();
    for (const std::vector<int> &clause : clauses) {
      for (int var : clause) cnf_file << var << " ";
      cnf_file << "0\n";
    }
  }

  cnf_file.close();
  return num_clauses;
}

}  // end namespace preimage