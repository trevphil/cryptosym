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

#include "hash_reversal/factor_graph.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace hash_reversal {

FactorGraph::FactorGraph(std::shared_ptr<Probability> prob,
                         std::shared_ptr<Dataset> dataset,
                         std::shared_ptr<utils::Config> config)
    : InferenceTool(prob, dataset, config) {}

InferenceTool::Prediction FactorGraph::predict(size_t rv_index) const {
  InferenceTool::Prediction prediction(rv_index, 0.5);
  double msg0 = 1.0, msg1 = 1.0;
  const auto &rv = rvs_.at(rv_index);

  for (size_t factor_index : rv.factor_indices) {
    msg0 *= factors_.at(factor_index).prevMessage(rv_index, 0);
    msg1 *= factors_.at(factor_index).prevMessage(rv_index, 1);
  }

  if (msg0 + msg1 == 0) {
    spdlog::warn("Prediction for RV {} would divide by zero!", rv_index);
  } else {
    prediction.prob_one = msg1 / (msg0 + msg1);
  }

  return prediction;
}

std::vector<InferenceTool::Prediction> FactorGraph::marginals() const {
  const size_t n = config_->num_rvs;
  std::vector<InferenceTool::Prediction> predictions;
  predictions.reserve(n);
  for (size_t rv = 0; rv < n; ++rv) predictions.push_back(predict(rv));
  return predictions;
}

void FactorGraph::reconfigure(const VariableAssignments &observed) {
  observed_ = observed;
  previous_marginals_.clear();
  for (auto &rv : rvs_) rv.reset();
  for (auto &factor : factors_) factor.reset(observed_, config_);
}

void FactorGraph::solve() {
  spdlog::info("\tStarting loopy BP...");
  const auto start = utils::Convenience::time_since_epoch();

  size_t itr = 0, forward = 0;
  for (itr = 0; itr < config_->lbp_max_iter; ++itr) {
    updateRandomVariableMessages(forward);
    updateFactorMessages(forward);
    const auto marg = marginals();
    if (equal(previous_marginals_, marg)) break;
    previous_marginals_ = marg;
    forward = (forward + 1) % 2;
  }

  if (itr >= config_->lbp_max_iter) {
    spdlog::warn("\tLoopy BP did not converge, max iterations reached.");
  } else {
    spdlog::info("\tLoopy BP converged in {} iterations", itr + 1);
  }

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("\tLBP finished in {} seconds.", end - start);
}

bool FactorGraph::equal(const std::vector<InferenceTool::Prediction> &marginals1,
                        const std::vector<InferenceTool::Prediction> &marginals2,
                        double tol) const {
  const size_t n = marginals1.size();
  if (n != marginals2.size()) return false;

  for (size_t i = 0; i < n; ++i) {
    const double p1 = marginals1.at(i).prob_one;
    const double p2 = marginals2.at(i).prob_one;
    if (std::abs(p1 - p2) > tol) return false;
  }

  return true;
}

void FactorGraph::updateFactorMessages(bool forward) {
  const size_t num_facs = factors_.size();

  for (size_t i = 0; i < num_facs; ++i) {
    const size_t factor_index = forward ? i : num_facs - i - 1;
    auto &factor = factors_.at(factor_index);

    for (size_t to_rv : factor.referenced_rvs) {
      std::set<size_t> unobserved_indices;
      for (size_t rv_index : factor.referenced_rvs) {
        if (observed_.count(rv_index) == 0 && rv_index != to_rv) {
          unobserved_indices.insert(rv_index);
        }
      }

      VariableAssignments assignments = observed_;
      const size_t n = unobserved_indices.size();
      const size_t num_combinations = 1 << n;
      double result0 = 0, result1 = 0;

      for (size_t combo = 0; combo < num_combinations; ++combo) {
        size_t i = 0;
        for (size_t unobserved_rv_index : unobserved_indices) {
          assignments[unobserved_rv_index] = (combo >> (i++)) & 1;
        }

        assignments[to_rv] = false;
        double message_product0 = prob_->probOne(factor, assignments, observed_);
        assignments[to_rv] = true;
        double message_product1 = prob_->probOne(factor, assignments, observed_);

        for (size_t rv_index : factor.referenced_rvs) {
          if (rv_index == to_rv) continue;
          const bool rv_val = assignments[rv_index];
          message_product0 *= rvs_.at(rv_index).prevMessage(factor_index, rv_val);
          message_product1 *= rvs_.at(rv_index).prevMessage(factor_index, rv_val);
        }
        result0 += message_product0;
        result1 += message_product1;
      }

      factor.updateMessage(to_rv, 0, result0, config_);
      factor.updateMessage(to_rv, 1, result1, config_);
    }
  }
}

void FactorGraph::updateRandomVariableMessages(bool forward) {
  const size_t num_rvs = config_->num_rvs;

  for (size_t i = 0; i < num_rvs; ++i) {
    const size_t rv_index = forward ? i : num_rvs - i - 1;
    auto &rv = rvs_.at(rv_index);
    for (size_t fac_idx : rv.factor_indices) {
      double result0 = 1.0;
      double result1 = 1.0;
      for (size_t other_fac_idx : rv.factor_indices) {
        if (fac_idx == other_fac_idx) continue;
        result0 *= factors_.at(other_fac_idx).prevMessage(rv_index, 0);
        result1 *= factors_.at(other_fac_idx).prevMessage(rv_index, 1);
      }
      rv.updateMessage(fac_idx, 0, result0, config_);
      rv.updateMessage(fac_idx, 1, result1, config_);
    }
  }
}

}  // end namespace hash_reversal