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

#include <vector>
#include <boost/dynamic_bitset.hpp>

#include "utils/config.hpp"
#include "hash_reversal/dataset.hpp"
#include "hash_reversal/probability.hpp"
#include "hash_reversal/factor_graph.hpp"

int main(int argc, char** argv) {
	if (argc < 2) {
		spdlog::error("You must provide a path to a YAML config file!");
		return -1;
	}

	const std::string config_file = argv[1];
	const utils::Config config = utils::Config(config_file);
	hash_reversal::Dataset dataset = hash_reversal::Dataset(config);
	hash_reversal::Probability prob(dataset, config);
	hash_reversal::FactorGraph factor_graph(prob, config);

	spdlog::info("Checking accuracy of single bit prediction on test data...");
	int correct_count = 0;
	int total_count = 0;
	std::vector<double> log_likelihood_ratios;
	std::vector<bool> accuracies;

	for (size_t test_idx = 0; test_idx < config.num_test_samples; ++test_idx) {
		const auto hash_bits = dataset.getHashBits(test_idx);
		const bool true_hash_input_bit = dataset.getGroundTruth(test_idx);
		const auto result = factor_graph.predict(config.bit_to_predict, 1, hash_bits);
		const bool guess = result.prob_bit_is_one > 0.5 ? true : false;
		const bool is_correct = (guess == true_hash_input_bit);
		correct_count += is_correct;
		++total_count;
		spdlog::info("\tGuessed {}, true value is {}", int(guess), int(true_hash_input_bit));

		log_likelihood_ratios.push_back(result.log_likelihood_ratio);
		accuracies.push_back(is_correct);

		spdlog::info("\tAccuracy: {0}/{1} ({2:.3f}%)",
			correct_count, total_count, 100.0 * correct_count / total_count);
	}

	return 0;
}