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

#include "hash_reversal/dataset.hpp"
#include "hash_reversal/factor_graph.hpp"
#include "hash_reversal/probability.hpp"
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

  const std::shared_ptr<hash_reversal::Dataset> dataset(new hash_reversal::Dataset(config));
  const std::shared_ptr<hash_reversal::Probability> prob(
      new hash_reversal::Probability(dataset, config));
  hash_reversal::FactorGraph factor_graph(prob, dataset, config);

  spdlog::info("Checking accuracy on test data...");
  int total_correct = 0;
  int total_count = 0;
  std::vector<double> num_correct_per_rv(config->num_rvs, 0);
  std::vector<double> count_per_rv(config->num_rvs, 0);

  const size_t num_test = std::min<size_t>(config->num_trials, config->num_test_samples);

  for (size_t test_idx = 0; test_idx < num_test; ++test_idx) {
    spdlog::info("Test case {}/{}", test_idx + 1, num_test);

    const auto observed_hash_bits = dataset->getHashBits(test_idx);
    factor_graph.runLBP(observed_hash_bits);
    const auto marginals = factor_graph.marginals();

    const auto ground_truth = dataset->getGroundTruth(test_idx);
    const size_t n = config->num_rvs;
    const size_t n_input = config->num_input_bits;
    size_t local_correct = 0, local_correct_hash_input = 0;
    double sum_abs_llr = 0;

    for (size_t rv = 0; rv < n; ++rv) {
      const auto prediction = marginals.at(rv);
      const bool guess = prediction.prob_bit_is_one > 0.5 ? true : false;
      const bool true_val = ground_truth[rv];
      const bool is_correct = (guess == true_val);
      total_correct += is_correct;
      local_correct += is_correct;
      if (dataset->isHashInputBit(rv)) {
        local_correct_hash_input += is_correct;
      }
      num_correct_per_rv[rv] += is_correct;
      count_per_rv[rv] += true_val;
      sum_abs_llr += std::abs(prediction.log_likelihood_ratio);
      total_count++;
    }

    spdlog::info("\tGot {0}/{1} ({2:.2f}%), average abs(LLR) is {3:.3f}", local_correct, n,
                 100.0 * local_correct / n, sum_abs_llr / n);

    const double hash_pct_correct = 100.0 * local_correct_hash_input / n_input;
    spdlog::info("\tGot {0}/{1} ({2:.2f}%) hash input bits", local_correct_hash_input, n_input,
                 hash_pct_correct);

    if (config->test_mode && hash_pct_correct < 90.0) {
      spdlog::error("Test case '{}': only {}/{} hash input bits predicted correctly",
                    config->hash_algo, local_correct_hash_input, n_input);
      return 1;
    }
  }

  if (config->print_bit_accuracies) {
    for (size_t rv = 0; rv < config->num_rvs; ++rv) {
      if (rv % 32 == 0 && rv != 0) spdlog::info("-----------------------------");
      spdlog::info("RV {0}:\taccuracy {1:.2f}%,\tmean {2:.2f}", rv + 1,
                   100.0 * num_correct_per_rv[rv] / num_test, count_per_rv[rv] / num_test);
    }
  }

  spdlog::info("Total accuracy: {0}/{1} ({2:.2f}%)", total_correct, total_count,
               100.0 * total_correct / total_count);
  spdlog::info("Done.");
  return 0;
}