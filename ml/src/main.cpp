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

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <cmath>
#include <memory>
#include <vector>

#include "hash_reversal/probability.hpp"
#include "hash_reversal/dataset.hpp"
#include "hash_reversal/inference_tool.hpp"
#include "hash_reversal/factor_graph.hpp"
#include "hash_reversal/bayes_net.hpp"
#include "utils/config.hpp"
#include "utils/stats.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    spdlog::error("You must provide a path to a YAML config file!");
    return 1;
  }

  // Derive the algorithm configuration from a YAML file
  const std::string config_file = argv[1];
  const std::shared_ptr<utils::Config> config(new utils::Config(config_file));
  if (!config->valid()) {
    spdlog::error("Invalid configuration, exiting.");
    return 1;
  }

  // Initialize objects used by the algorithm
  const std::shared_ptr<hash_reversal::Dataset> dataset(
			new hash_reversal::Dataset(config));
  const std::shared_ptr<hash_reversal::Probability> prob(
    	new hash_reversal::Probability(config));

  std::shared_ptr<hash_reversal::InferenceTool> inference_tool;

  if (config->method == "lbp") {
    inference_tool = std::shared_ptr<hash_reversal::InferenceTool>(
        new hash_reversal::FactorGraph(prob, dataset, config));
  } else if (config->method == "gtsam") {
    inference_tool = std::shared_ptr<hash_reversal::InferenceTool>(
        new hash_reversal::BayesNet(prob, dataset, config));
  } else {
    spdlog::error("Unsupported method: {}", config->method);
    return 1;
  }

  spdlog::info("Checking accuracy on test data...");
  const size_t n = config->num_rvs;
  const size_t n_input = config->input_rv_indices.size();

  // Initialize a helper object to track statistics while running the algo
  std::vector<std::string> f_types(n);
  for (size_t rv = 0; rv < n; ++rv) f_types[rv] = inference_tool->factorType(rv);
  utils::Stats stats(config, f_types);

  // How many hash input --> hash output trials to run
  const size_t num_test = std::min<size_t>(config->num_test,
                                           config->num_samples);

  for (size_t sample_idx = 0; sample_idx < num_test; ++sample_idx) {
    spdlog::info("Test case {}/{}", sample_idx + 1, num_test);

    const auto observed = dataset->getObservedData(sample_idx);
    inference_tool->reconfigure(observed);
    inference_tool->solve();
    const auto marginals = inference_tool->marginals();
    boost::dynamic_bitset<> predicted_input(n_input);

    const auto ground_truth = dataset->getFullSample(sample_idx);

    for (size_t rv = 0; rv < n; ++rv) {
      const double p = marginals.at(rv).prob_one;
      const bool predicted_val = p > 0.5 ? true : false;
      const bool true_val = ground_truth[rv];
      const bool is_observed = observed.count(rv) > 0;
      stats.update(rv, predicted_val, true_val, p, is_observed);

      if (dataset->isHashInputBit(rv)) {
        predicted_input[rv] = predicted_val;
      }
    }

    // Verify the predicted input creates a hash collision / pre-image
    const bool valid = dataset->validate(predicted_input, sample_idx);
    if (config->test_mode && !valid) return 1;
  }

  stats.save();

  spdlog::info("Done.");
  return 0;
}