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

Solver::Solver(const std::map<size_t, Factor> &factors,
               const std::vector<size_t> &input_indices)
    : factors_(factors), input_indices_(input_indices) {}

Solver::~Solver() {}

std::map<size_t, bool> Solver::solve(const std::map<size_t, bool> &observed) {
  const auto start = Utils::sec_since_epoch();
  reset();
  observed_ = observed;
  setImplicitObserved();
  const auto solution = solveInternal();
  const auto end = Utils::sec_since_epoch();
  spdlog::info("Solution found in {} s", end - start);
  return solution;
}

void Solver::reset() { observed_.clear(); }

void Solver::setImplicitObserved() {
  const size_t initial = observed_.size();
  spdlog::info("There are {} initially observed bits", initial);
  while (true) {
    const size_t before = observed_.size();
    const size_t smallest_obs = propagateBackward();
    propagateForward(smallest_obs);
    const size_t after = observed_.size();
    if (before == after) break;
  }
  const size_t diff = observed_.size() - initial;
  spdlog::info("Pre-solved for {} additional bits", diff);
}

size_t Solver::propagateBackward() {
  std::list<size_t> queue;
  for (const auto &itr : observed_) queue.push_back(itr.first);
  size_t smallest_obs = factors_.size();
  size_t inp, inp1, inp2;
  bool inp1_obs, inp2_obs;

  while (queue.size() > 0) {
    const size_t rv = queue.front();
    queue.pop_front();

    if (factors_.count(rv) == 0) continue;
    const Factor &f = factors_.at(rv);
    if (!f.valid) continue;

    switch (f.t) {
    case Factor::Type::SameFactor:
      inp = f.inputs.at(0);
      observed_[inp] = observed_.at(rv);
      queue.push_back(inp);
      smallest_obs = std::min(smallest_obs, inp);
      break;
    case Factor::Type::NotFactor:
      inp = f.inputs.at(0);
      observed_[inp] = !observed_.at(rv);
      queue.push_back(inp);
      smallest_obs = std::min(smallest_obs, inp);
      break;
    case Factor::Type::AndFactor:
      inp1 = f.inputs.at(0);
      inp2 = f.inputs.at(1);
      if (observed_.at(rv) == true) {
        // Output is observed to be 1, so both inputs must have been 1
        observed_[inp1] = true;
        observed_[inp2] = true;
        queue.push_back(inp1);
        queue.push_back(inp2);
        smallest_obs = std::min(inp1, std::min(inp2, smallest_obs));
      } else if (observed_.count(inp1) > 0 && observed_.at(inp1) == true) {
        // Output is 0, inp1 is 1, so inp2 must be 0
        observed_[inp2] = false;
        queue.push_back(inp2);
        smallest_obs = std::min(smallest_obs, inp2);
      } else if (observed_.count(inp2) > 0 && observed_.at(inp2) == true) {
        // Output is 0, inp2 is 1, so inp1 must be 0
        observed_[inp1] = false;
        queue.push_back(inp1);
        smallest_obs = std::min(smallest_obs, inp1);
      }
      break;
    case Factor::Type::XorFactor:
      inp1 = f.inputs.at(0);
      inp2 = f.inputs.at(1);
      inp1_obs = observed_.count(inp1) > 0;
      inp2_obs = observed_.count(inp2) > 0;
      if (inp1_obs && !inp2_obs) {
        observed_[inp2] = observed_.at(rv) ^ observed_.at(inp1);
        queue.push_back(inp2);
        smallest_obs = std::min(smallest_obs, inp2);
      } else if (inp2_obs && !inp1_obs) {
        observed_[inp1] = observed_.at(rv) ^ observed_.at(inp2);
        queue.push_back(inp1);
        smallest_obs = std::min(smallest_obs, inp1);
      }
      break;
    case Factor::Type::OrFactor:
      inp1 = f.inputs.at(0);
      inp2 = f.inputs.at(1);
      if (observed_.at(rv) == false) {
        // Output is observed to be 0, so both inputs must have been 0
        observed_[inp1] = false;
        observed_[inp2] = false;
        queue.push_back(inp1);
        queue.push_back(inp2);
        smallest_obs = std::min(inp1, std::min(inp2, smallest_obs));
      } else if (observed_.count(inp1) > 0 && observed_.at(inp1) == false) {
        // Output is 1, inp1 is 0, so inp2 must be 1
        observed_[inp2] = true;
        queue.push_back(inp2);
        smallest_obs = std::min(smallest_obs, inp2);
      } else if (observed_.count(inp2) > 0 && observed_.at(inp2) == false) {
        // Output is 1, inp2 is 0, so inp1 must be 1
        observed_[inp1] = true;
        queue.push_back(inp1);
        smallest_obs = std::min(smallest_obs, inp1);
      }
      break;
    case Factor::Type::PriorFactor:
      break;
    }
  }

  return smallest_obs;
}

void Solver::propagateForward(size_t smallest_obs) {
  const size_t n = factors_.size();
  size_t inp, inp1, inp2;
  bool inp1_obs, inp2_obs;

  for (size_t rv = smallest_obs; rv < n; rv++) {
    if (observed_.count(rv) > 0) continue;
    if (factors_.count(rv) == 0) continue;
    const Factor &f = factors_.at(rv);
    if (!f.valid) continue;

    switch (f.t) {
    case Factor::Type::SameFactor:
      inp = f.inputs.at(0);
      if (observed_.count(inp) > 0) observed_[rv] = observed_.at(inp);
      break;
    case Factor::Type::NotFactor:
      inp = f.inputs.at(0);
      if (observed_.count(inp) > 0) observed_[rv] = !observed_.at(inp);
      break;
    case Factor::Type::AndFactor:
      inp1 = f.inputs.at(0);
      inp2 = f.inputs.at(1);
      inp1_obs = observed_.count(inp1) > 0;
      inp2_obs = observed_.count(inp2) > 0;
      if (inp1_obs && inp2_obs) {
        observed_[rv] = observed_.at(inp1) & observed_.at(inp2);
      } else if (inp1_obs && observed_.at(inp1) == false) {
        observed_[rv] = false;
      } else if (inp2_obs && observed_.at(inp2) == false) {
        observed_[rv] = false;
      }
      break;
    case Factor::Type::XorFactor:
      inp1 = f.inputs.at(0);
      inp2 = f.inputs.at(1);
      if (observed_.count(inp1) > 0 && observed_.count(inp2) > 0) {
        observed_[rv] = observed_.at(inp1) ^ observed_.at(inp2);
      }
      break;
    case Factor::Type::OrFactor:
      inp1 = f.inputs.at(0);
      inp2 = f.inputs.at(1);
      inp1_obs = observed_.count(inp1) > 0;
      inp2_obs = observed_.count(inp2) > 0;
      if (inp1_obs && inp2_obs) {
        observed_[rv] = observed_.at(inp1) | observed_.at(inp2);
      } else if (inp1_obs && observed_.at(inp1) == true) {
        observed_[rv] = true;
      } else if (inp2_obs && observed_.at(inp2) == true) {
        observed_[rv] = true;
      }
      break;
    case Factor::Type::PriorFactor:
      break;
    }
  }
}

}  // end namespace preimage