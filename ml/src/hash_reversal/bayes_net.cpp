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

#include "hash_reversal/bayes_net.hpp"

#include <spdlog/spdlog.h>

namespace hash_reversal {

using namespace gtsam;

BayesNet::BayesNet(std::shared_ptr<Probability> prob,
                   std::shared_ptr<Dataset> dataset,
                   std::shared_ptr<utils::Config> config)
    : InferenceTool(prob, dataset, config) {
  for (const auto &factor : factors_) {
    gtsam::DiscreteKey out(factor.output_rv, 2);
    std::vector<gtsam::DiscreteKey> inp;
    for (const size_t input : factor.referenced_rvs) {
      if (input == factor.output_rv) continue;
      inp.push_back(gtsam::DiscreteKey(input, 2));
    }

    ordering_ += out.first;

    if (factor.factor_type == "PRIOR") {
      dbn_.add(out % "50/50");
    } else if (factor.factor_type == "AND") {
      dbn_.add((out | inp.at(0), inp.at(1)) = "1/0 1/0 1/0 0/1");
    } else if (factor.factor_type == "OR") {
      dbn_.add((out | inp.at(0), inp.at(1)) = "1/0 0/1 0/1 0/1");
    } else if (factor.factor_type == "XOR") {
      dbn_.add((out | inp.at(0), inp.at(1)) = "1/0 0/1 0/1 1/0");
    } else if (factor.factor_type == "INV") {
      dbn_.add((out | inp.at(0)) = "0/1 1/0");
    } else {
      spdlog::error("Unsupported factor: {}", factor.factor_type);
    }
  }

  factor_graph_ = gtsam::DiscreteFactorGraph(dbn_);
}

void BayesNet::update(const VariableAssignments &observed) {
  for (const auto it : observed) {
    const gtsam::DiscreteKey key(it.first, 2);
    const std::string val = it.second ? "0 1" : "1 0";
    factor_graph_.add(key, val);
  }

  // Can also try it without giving an ordering
  // auto bayes_tree = factor_graph_.eliminateMultifrontal(ordering_);
  auto chordal = factor_graph_.eliminateSequential(ordering_);
  gtsam::DiscreteFactor::sharedValues mpe = chordal->optimize();

  const size_t n = config_->num_rvs;
  predictions_.clear();
  predictions_.reserve(n);
  for (size_t rv = 0; rv < n; ++rv) {
    const double prob_one = mpe->at(rv) == 1 ? 1.0 : 0.0;
    const InferenceTool::Prediction pred(rv, prob_one);
    predictions_.push_back(pred);
  }
}

std::vector<InferenceTool::Prediction> BayesNet::marginals() const {
  return predictions_;
}

void BayesNet::reset() {
  predictions_.clear();
  factor_graph_ = gtsam::DiscreteFactorGraph(dbn_);
}

}  // end namespace hash_reversal