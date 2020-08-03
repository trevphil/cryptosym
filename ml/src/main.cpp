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

#include "hash_reversal/factor_graph.hpp"
#include "hash_reversal/probability.hpp"
#include "hash_reversal/dataset.hpp"
#include "utils/config.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    spdlog::error("You must provide a path to a YAML config file!");
    return 1;
  }

  const std::string config_file = argv[1];
  const std::shared_ptr<utils::Config> config(new utils::Config(config_file));
  if (!config->valid()) {
    spdlog::error("Invalid configuration, exiting.");
    return 1;
  }

  const std::shared_ptr<hash_reversal::Dataset> dataset(
			new hash_reversal::Dataset(config));
  const std::shared_ptr<hash_reversal::Probability> prob(
    	new hash_reversal::Probability(config));
  hash_reversal::FactorGraph factor_graph(prob, dataset, config);

  spdlog::info("Checking accuracy on test data...");
  int total_correct = 0;
  int total_count = 0;
  const size_t n = config->num_rvs;
  const size_t n_input = config->input_rv_indices.size();
  std::vector<double> num_correct_per_rv(n, 0);
  std::vector<double> count_per_rv(n, 0);

  const size_t num_test = 16; // config->num_samples;

  for (size_t sample_idx = 0; sample_idx < num_test; ++sample_idx) {
    spdlog::info("Test case {}/{}", sample_idx + 1, num_test);

    const auto observed = dataset->getObservedData(sample_idx);
    factor_graph.runLBP(observed);
    const auto marginals = factor_graph.marginals();
    boost::dynamic_bitset<> predicted_input(n_input);

    const auto ground_truth = dataset->getFullSample(sample_idx);
    size_t local_correct = 0, local_correct_hash_input = 0;

    for (size_t rv = 0; rv < n; ++rv) {
      const auto prediction = marginals.at(rv);
      const bool guess = prediction.prob_one > 0.5 ? true : false;
      const bool true_val = ground_truth[rv];
      const bool is_correct = (guess == true_val);
      total_correct += is_correct;
      local_correct += is_correct;
      if (dataset->isHashInputBit(rv)) {
        local_correct_hash_input += is_correct;
        predicted_input[prediction.rv_index] = guess;
      }
      num_correct_per_rv[rv] += is_correct;
      count_per_rv[rv] += true_val;
      total_count++;
    }

    spdlog::info("\tInput          : {}", dataset->getHashInput(sample_idx));
    spdlog::info("\tPredicted input: {}", utils::Convenience::bitset2hex(predicted_input));
    spdlog::info("\tHash           : {}", dataset->getHash(sample_idx));

    const double input_pct_correct = 100.0 * local_correct_hash_input / n_input;
    spdlog::info("\tGot {0}/{1} ({2:.2f}%) hash input bits", local_correct_hash_input, n_input,
                 input_pct_correct);
    spdlog::info("\tGot {0}/{1} ({2:.2f}%)", local_correct, n, 100.0 * local_correct / n);

    if (config->test_mode && input_pct_correct < 90.0) {
      spdlog::error("Test case '{}': only {}/{} hash input bits predicted correctly",
                    config->hash_algo, local_correct_hash_input, n_input);
      return 1;
    }
  }

  if (config->print_bit_accuracies) {
    for (const size_t &rv : config->input_rv_indices) {
      spdlog::info("RV {0} ({1}):\taccuracy {2:.2f}%,\tmean value {3:.2f}",
                   rv, factor_graph.factorType(rv),
                   100.0 * num_correct_per_rv[rv] / num_test,
                   count_per_rv[rv] / num_test);
    }
  }

  spdlog::info("Total accuracy: {0}/{1} ({2:.2f}%)", total_correct, total_count,
               100.0 * total_correct / total_count);
  spdlog::info("Done.");
  return 0;
}