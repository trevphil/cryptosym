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

#include <iostream>

#include "spdlog/spdlog.h"

#include "hash_reversal/probability.hpp"

namespace hash_reversal {

Probability::Probability(std::shared_ptr<Dataset> dataset,
												 std::shared_ptr<utils::Config> config) : dataset_(dataset) {
	spdlog::info("Initializing marginals...");
	const auto start = utils::Convenience::time_since_epoch();

	const double num_samples = config->num_train_samples;
	phats_ = Eigen::MatrixXd(config->num_rvs, 2);
	for (size_t rv = 0; rv < config->num_rvs; ++rv) {
		double prob_zero = count({VariableAssignment(rv, 0)}) / num_samples;
		phats_(rv, 0) = prob_zero;
		phats_(rv, 1) = 1.0 - prob_zero;
	}

	const auto end = utils::Convenience::time_since_epoch();
	spdlog::info("Marginals initialized in {} seconds.", end - start);
}

size_t Probability::count(const std::vector<VariableAssignment> &rv_assignments) const {
	if (!rv_assignments.size()) {
		spdlog::info("Cannot count random variable assignments if none are given!");
		return 0;
	}

	auto rva = rv_assignments[0];
	boost::dynamic_bitset<> filter = dataset_->getTrainSamples(rva.rv_index);
	if (rva.value == 0) filter = ~filter;

	for (size_t i = 1; i < rv_assignments.size(); ++i) {
		rva = rv_assignments[i];
		boost::dynamic_bitset<> samples = dataset_->getTrainSamples(rva.rv_index);
		filter = filter & (rva.value == 0 ? ~samples : samples);
	}

	return filter.count();
}

}  // end namespace hash_reversal