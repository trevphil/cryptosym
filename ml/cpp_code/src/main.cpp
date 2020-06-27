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

#include <memory>
#include <vector>
#include <boost/dynamic_bitset.hpp>

#include "utils/config.hpp"
#include "hash_reversal/dataset.hpp"
#include "hash_reversal/probability.hpp"
#include "hash_reversal/factor_graph.hpp"

/*
TODO -
  Plot accuracies and LLR

  Think about what would be a natural BN structure...
    -> Should all hash bits in a byte be fully connected?
    -> Stuff happens in groups of 32 bits, so it makes sense to use 32 connections per node

  Technically the test data is used in "training" in MATLAB
*/

int main(int argc, char** argv) {
	if (argc < 2) {
		spdlog::error("You must provide a path to a YAML config file!");
		return -1;
	}

	const std::string config_file = argv[1];
	const std::shared_ptr<utils::Config> config(
		new utils::Config(config_file));
	const std::shared_ptr<hash_reversal::Dataset> dataset(
		new hash_reversal::Dataset(config));
	const std::shared_ptr<hash_reversal::Probability> prob(
		new hash_reversal::Probability(dataset, config));
	hash_reversal::FactorGraph factor_graph(prob, config);

	spdlog::info("Checking accuracy of single bit prediction on test data...");
	int correct_count = 0;
	int total_count = 0;
	std::vector<double> log_likelihood_ratios;
	std::vector<bool> accuracies;

	for (size_t test_idx = 0; test_idx < config->num_test_samples; ++test_idx) {
		const auto hash_bits = dataset->getHashBits(test_idx);
		const bool true_hash_input_bit = dataset->getGroundTruth(test_idx);
		const auto result = factor_graph.predict(config->bit_to_predict, hash_bits);
		const bool guess = result.prob_bit_is_one > 0.5 ? true : false;
		const bool is_correct = (guess == true_hash_input_bit);
		correct_count += is_correct;
		++total_count;
		spdlog::info("\tGuessed {}, true value is {}, LLR {}",
			int(guess), int(true_hash_input_bit), result.log_likelihood_ratio);

		log_likelihood_ratios.push_back(result.log_likelihood_ratio);
		accuracies.push_back(is_correct);

		spdlog::info("\tAccuracy: {0}/{1} ({2:.3f}%)",
			correct_count, total_count, 100.0 * correct_count / total_count);
	}

	spdlog::info("Done.");
	return 0;
}