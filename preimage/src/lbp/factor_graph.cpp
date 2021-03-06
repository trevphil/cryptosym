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

#include "lbp/factor_graph.hpp"
#include "utils.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace preimage {

namespace lbp {

FactorGraph::FactorGraph(const std::map<size_t, Factor> &factors,
                         const std::vector<size_t> &input_indices)
    : Solver(factors, input_indices), params_(), prob_(params_) {
  spdlog::info("Initializing loopy belief propagation...");
  rvs_ = {};
  lbp_factors_ = {};
  for (const auto &itr : factors) {
    const size_t rv_index = itr.first;
    const Factor &f = itr.second;
    if (!f.valid) continue;

    RandomVariable rv;
    rv.factor_indices = f.inputs;
    rv.factor_indices.push_back(rv_index);
    rvs_[rv_index] = rv;

    LbpFactor lbp_factor(f);
    lbp_factors_[rv_index] = lbp_factor;
  }
  spdlog::info("Did initialize (n={})", rvs_.size());
}

FactorGraph::Prediction FactorGraph::predict(size_t rv_index) const {
  Prediction prediction(rv_index, 0.5);
  double msg0 = 1.0, msg1 = 1.0;
  const auto &rv = rvs_.at(rv_index);

  for (size_t factor_index : rv.factor_indices) {
    msg0 *= lbp_factors_.at(factor_index).prevMessage(rv_index, 0);
    msg1 *= lbp_factors_.at(factor_index).prevMessage(rv_index, 1);
  }

  if (msg0 + msg1 == 0) {
    spdlog::warn("Prediction for RV {} would divide by zero!", rv_index);
  } else {
    prediction.prob_one = msg1 / (msg0 + msg1);
  }

  return prediction;
}

std::vector<FactorGraph::Prediction> FactorGraph::marginals() const {
  std::vector<FactorGraph::Prediction> predictions;
  predictions.reserve(lbp_factors_.size());
  for (auto &itr : lbp_factors_) predictions.push_back(predict(itr.first));
  return predictions;
}

void FactorGraph::reset() {
  Solver::reset();
  previous_marginals_.clear();
  for (auto &itr : rvs_) itr.second.clearMessages();
  for (auto &itr : lbp_factors_) itr.second.clearMessages();
}

std::map<size_t, bool> FactorGraph::solveInternal() {
  spdlog::info("\tStarting loopy BP...");
  const auto start = Utils::time_since_epoch();

  size_t itr = 0, forward = 0;
  for (itr = 0; itr < params_.max_iter; ++itr) {
    updateRandomVariableMessages(forward);
    updateFactorMessages(forward);
    const auto marg = marginals();
    if (equal(previous_marginals_, marg)) break;
    previous_marginals_ = marg;
    forward = (forward + 1) % 2;
  }

  if (itr >= params_.max_iter) {
    spdlog::warn("\tLoopy BP did not converge, max iterations reached.");
  } else {
    spdlog::info("\tLoopy BP converged in {} iterations", itr + 1);
  }

  const auto end = Utils::time_since_epoch();
  spdlog::info("\tLBP finished in {} seconds.", end - start);

  std::map<size_t, bool> solution;
  for (const auto &prediction : previous_marginals_) {
    solution[prediction.rv_index] = (prediction.prob_one > 0.5);
  }
  return solution;
}

bool FactorGraph::equal(const std::vector<FactorGraph::Prediction> &marginals1,
                        const std::vector<FactorGraph::Prediction> &marginals2) const {
  const size_t n = marginals1.size();
  if (n != marginals2.size()) return false;

  for (size_t i = 0; i < n; ++i) {
    const size_t rv1 = marginals1.at(i).rv_index;
    const size_t rv2 = marginals2.at(i).rv_index;
    if (rv1 != rv2) return false;
    const double p1 = marginals1.at(i).prob_one;
    const double p2 = marginals2.at(i).prob_one;
    if (std::abs(p1 - p2) > params_.convergence_tol) return false;
  }

  return true;
}

void FactorGraph::updateFactorMessages(bool forward) {
  auto fwd_itr = lbp_factors_.begin();
  auto rev_itr = lbp_factors_.rbegin();

  for (size_t idx = 0; idx < lbp_factors_.size(); ++idx) {
    auto &factor = forward ? fwd_itr->second : rev_itr->second;
    const size_t factor_index = forward ? fwd_itr->first : rev_itr->first;
    if (forward) {
      ++fwd_itr;
    } else {
      ++rev_itr;
    }

    for (size_t to_rv : factor.referenced_rvs) {
      std::set<size_t> unobserved_indices;
      for (size_t rv_index : factor.referenced_rvs) {
        if (observed_.count(rv_index) == 0 && rv_index != to_rv) {
          unobserved_indices.insert(rv_index);
        }
      }

      std::map<size_t, bool> assignments = observed_;
      const size_t n = unobserved_indices.size();
      const size_t num_combinations = 1 << n;
      double result0 = 0, result1 = 0;

      for (size_t combo = 0; combo < num_combinations; ++combo) {
        size_t i = 0;
        for (size_t unobserved_rv_index : unobserved_indices) {
          assignments[unobserved_rv_index] = (combo >> (i++)) & 1;
        }

        assignments[to_rv] = false;
        double message_product0 = prob_.probOne(factor, assignments, observed_);
        assignments[to_rv] = true;
        double message_product1 = prob_.probOne(factor, assignments, observed_);

        for (size_t rv_index : factor.referenced_rvs) {
          if (rv_index == to_rv) continue;
          const bool rv_val = assignments[rv_index];
          // does this make sense?
          message_product0 *= rvs_.at(rv_index).prevMessage(factor_index, rv_val);
          message_product1 *= rvs_.at(rv_index).prevMessage(factor_index, rv_val);
        }
        result0 += message_product0;
        result1 += message_product1;
      }

      factor.updateMessage(to_rv, 0, result0, params_);
      factor.updateMessage(to_rv, 1, result1, params_);
    }
  }
}

void FactorGraph::updateRandomVariableMessages(bool forward) {
  auto fwd_itr = rvs_.begin();
  auto rev_itr = rvs_.rbegin();

  for (size_t idx = 0; idx < rvs_.size(); ++idx) {
    auto &rv = forward ? fwd_itr->second : rev_itr->second;
    const size_t rv_index = forward ? fwd_itr->first : rev_itr->first;
    if (forward) {
      ++fwd_itr;
    } else {
      ++rev_itr;
    }

    for (size_t fac_idx : rv.factor_indices) {
      double result0 = 1.0;
      double result1 = 1.0;
      for (size_t other_fac_idx : rv.factor_indices) {
        if (fac_idx == other_fac_idx) continue;
        result0 *= lbp_factors_.at(other_fac_idx).prevMessage(rv_index, 0);
        result1 *= lbp_factors_.at(other_fac_idx).prevMessage(rv_index, 1);
      }
      rv.updateMessage(fac_idx, 0, result0, params_);
      rv.updateMessage(fac_idx, 1, result1, params_);
    }
  }
}

}  // end namespace lbp

}  // end namespace preimage