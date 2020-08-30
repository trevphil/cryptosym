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

#include <algorithm>

#include <spdlog/spdlog.h>

namespace hash_reversal {

using gtsam::symbol_shorthand::X;

BayesNet::BayesNet(std::shared_ptr<Probability> prob,
                   std::shared_ptr<Dataset> dataset,
                   std::shared_ptr<utils::Config> config)
    : InferenceTool(prob, dataset, config) {
  noise_ = gtsam::noiseModel::Diagonal::Sigmas(
      (gtsam::Vector(1) << 100.0).finished());
  prior_noise_ = gtsam::noiseModel::Diagonal::Sigmas(
      (gtsam::Vector(1) << 1e-6).finished());
}

void BayesNet::reconfigure(const VariableAssignments &observed) {
  observed_ = observed;
  predictions_.clear();
  graph_.resize(0u);
  init_values_.clear();
}

void BayesNet::solve() {
  spdlog::info("\tStarting GTSAM method...");
  const auto start = utils::Convenience::time_since_epoch();

  for (const auto &factor : factors_) {
    const std::vector<size_t> inputs = factor.inputRVs();
    const size_t rv = factor.output_rv;
    const std::string &ftype = factor.factor_type;
    const bool is_observed = observed_.count(rv) > 0;
    const double val = is_observed ? double(observed_.at(rv)) : 0.5;
    init_values_.insert(X(rv), val);

    if (ftype == "PRIOR") {
      gtsam::PriorFactor<double> prior(X(rv), val, noise_);
      graph_.add(prior);
    } else if (ftype == "SAME") {
      SameFactor same_fac(X(inputs.at(0)), X(rv), noise_);
      graph_.add(same_fac);
    } else if (ftype == "INV") {
      InvFactor inv_fac(X(inputs.at(0)), X(rv), noise_);
      graph_.add(inv_fac);
    } else if (ftype == "AND") {
      AndFactor and_fac(X(inputs.at(0)), X(inputs.at(1)), X(rv), noise_);
      graph_.add(and_fac);
    } else {
      spdlog::error("\tUnsupported factor type: {}", ftype);
    }
  }

  for (const auto it : observed_) {
    // Attach priors with extremely low variance to the observed RVs
    gtsam::PriorFactor<double> prior(
        X(it.first), double(it.second), prior_noise_);
    graph_.add(prior);
  }

  spdlog::info("\tStarting graph optimization...");
  gtsam::LevenbergMarquardtParams params;
  params.maxIterations = 20;
  params.relativeErrorTol = 1e-4;
  params.absoluteErrorTol = 1e-4;
  gtsam::LevenbergMarquardtOptimizer optimizer(graph_, init_values_, params);
  const gtsam::Values result = optimizer.optimize();
  spdlog::info("\tFinished graph optimization in {} iterations (error={})",
               optimizer.iterations(), optimizer.error());

  const size_t n = config_->num_rvs;
  predictions_.clear();
  predictions_.reserve(n);
  for (size_t rv = 0; rv < n; ++rv) {
    const double rv_val = result.at<double>(X(rv));
    // spdlog::info("RV {}: {}", rv, rv_val);
    const double prob_one = std::max(0.0, std::min(1.0, rv_val));
    const InferenceTool::Prediction pred(rv, prob_one);
    predictions_.push_back(pred);
  }

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("\tGTSAM method finished in {} seconds.", end - start);
}

std::vector<InferenceTool::Prediction> BayesNet::marginals() const {
  return predictions_;
}

}  // end namespace hash_reversal