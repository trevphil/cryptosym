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

void Solver::setFactors(const std::map<size_t, Factor> &factors) {
  factors_ = factors;
}

void Solver::setInputIndices(const std::vector<size_t> &input_indices) {
  input_indices_ = input_indices;
}

void Solver::setObserved(const std::map<size_t, bool> &observed) {
  observed_ = observed;
}

std::map<size_t, bool> Solver::solve() {
  const auto start = Utils::ms_since_epoch();
  setImplicitObserved();
  initialize();
  const auto solution = solveInternal();
  const auto end = Utils::ms_since_epoch();
  if (verbose_) spdlog::info("Solver finished in {} ms", end - start);
  return solution;
}

void Solver::setImplicitObserved() {
  const size_t initial = observed_.size();
  if (verbose_) spdlog::info("There are {} initially observed bits", initial);
  while (true) {
    const size_t before = observed_.size();
    const size_t smallest_obs = propagateBackward();
    propagateForward(smallest_obs);
    const size_t after = observed_.size();
    if (before == after) break;
  }
  const size_t diff = observed_.size() - initial;
  if (verbose_) spdlog::info("Pre-solved for {} additional bits", diff);
}

size_t Solver::propagateBackward() {
  std::list<size_t> queue;
  for (const auto &itr : observed_) queue.push_back(itr.first);
  size_t smallest_obs = factors_.size() * 2;
  size_t inp, inp1, inp2, inp3;
  bool inp1_obs, inp2_obs, inp3_obs;

  while (queue.size() > 0) {
    const size_t rv = queue.front();
    queue.pop_front();

    if (factors_.count(rv) == 0) continue;
    const Factor &f = factors_.at(rv);
    if (!f.valid) continue;

    switch (f.t) {
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
    case Factor::Type::MajFactor:
      inp1 = f.inputs.at(0);
      inp2 = f.inputs.at(1);
      inp3 = f.inputs.at(2);
      inp1_obs = observed_.count(inp1) > 0;
      inp2_obs = observed_.count(inp2) > 0;
      inp3_obs = observed_.count(inp3) > 0;
      if (observed_.at(rv) == true) {
        if (inp1_obs && observed_.at(inp1) == false) {
          observed_[inp2] = true;
          observed_[inp3] = true;
        } else if (inp2_obs && observed_.at(inp2) == false) {
          observed_[inp1] = true;
          observed_[inp3] = true;
        } else if (inp3_obs && observed_.at(inp3) == false) {
          observed_[inp1] = true;
          observed_[inp2] = true;
        }
      } else {  // Else observed_.at(rv) = false
        if (inp1_obs && observed_.at(inp1) == true) {
          observed_[inp2] = false;
          observed_[inp3] = false;
        } else if (inp2_obs && observed_.at(inp2) == true) {
          observed_[inp1] = false;
          observed_[inp3] = false;
        } else if (inp3_obs && observed_.at(inp3) == true) {
          observed_[inp1] = false;
          observed_[inp2] = false;
        }
      }
      break;
    }
  }

  return smallest_obs;
}

void Solver::propagateForward(size_t smallest_obs) {
  const size_t n = factors_.size() * 2;
  size_t inp, inp1, inp2, inp3;
  bool inp1_obs, inp2_obs, inp3_obs;

  for (size_t rv = smallest_obs; rv < n; rv++) {
    if (observed_.count(rv) > 0) continue;
    if (factors_.count(rv) == 0) continue;
    const Factor &f = factors_.at(rv);
    if (!f.valid) continue;

    switch (f.t) {
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
    case Factor::Type::MajFactor:
      inp1 = f.inputs.at(0);
      inp2 = f.inputs.at(1);
      inp3 = f.inputs.at(2);
      inp1_obs = observed_.count(inp1) > 0;
      inp2_obs = observed_.count(inp2) > 0;
      inp3_obs = observed_.count(inp3) > 0;
      if (inp1_obs && inp2_obs && inp3_obs) {
        observed_[rv] = ((int)observed_.at(inp1) +
                         (int)observed_.at(inp2) +
                         (int)observed_.at(inp3)) > 1;
      } else if (inp1_obs && inp2_obs &&
                 observed_.at(inp1) == observed_.at(inp2)) {
        observed_[rv] = observed_.at(inp1);
      } else if (inp1_obs && inp3_obs &&
                 observed_.at(inp1) == observed_.at(inp3)) {
        observed_[rv] = observed_.at(inp1);
      } else if (inp2_obs && inp3_obs &&
                 observed_.at(inp2) == observed_.at(inp3)) {
        observed_[rv] = observed_.at(inp2);
      }
      break;
    }
  }
}

}  // end namespace preimage