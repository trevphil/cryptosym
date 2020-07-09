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

#include <cmath>
#include <memory>
#include <vector>
#include <algorithm>
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
	const std::shared_ptr<utils::Config> config(
		new utils::Config(config_file));
	const std::shared_ptr<hash_reversal::Dataset> dataset(
		new hash_reversal::Dataset(config));
	const std::shared_ptr<hash_reversal::Probability> prob(
		new hash_reversal::Probability(dataset, config));
	hash_reversal::FactorGraph factor_graph(prob, config);

	spdlog::info("Checking accuracy on test data...");
	int total_correct = 0;
	int total_count = 0;
	std::vector<double> log_likelihood_ratios;
	std::vector<bool> accuracies;

	const size_t num_test = std::min<size_t>(1, config->num_test_samples);

	for (size_t test_idx = 0; test_idx < num_test; ++test_idx) {
		const auto observed_hash_bits = dataset->getHashBits(test_idx);
		factor_graph.runLBP(observed_hash_bits);

		const auto hash_input = dataset->getGroundTruth(test_idx);
		const size_t n = hash_input.size();
		int local_correct = 0;
		double sum_abs_llr = 0;

		for (size_t i = 0; i < n; ++i) {
			const size_t bit_index = config->num_hash_bits + i;
			const auto result = factor_graph.predict(bit_index);
			const bool guess = result.prob_bit_is_one > 0.5 ? true : false;
			const bool is_correct = (guess == hash_input[i]);
			total_correct += is_correct;
			local_correct += is_correct;
			++total_count;
			sum_abs_llr += std::abs(result.log_likelihood_ratio);
			log_likelihood_ratios.push_back(result.log_likelihood_ratio);
			accuracies.push_back(is_correct);
		}

		const double local_pct_correct = 100.0 * local_correct / n;
		spdlog::info("\tGot {0}/{1} ({2:.2f}%), average abs(LLR) is {3:.3f}",
			local_correct, n, local_pct_correct, sum_abs_llr / n);
		spdlog::info("\tTotal accuracy: {0}/{1} ({2:.2f}%)",
			total_correct, total_count, 100.0 * total_correct / total_count);
	}

	spdlog::info("Done.");
	return 0;
}