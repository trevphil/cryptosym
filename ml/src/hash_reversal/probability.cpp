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

#include <spdlog/spdlog.h>

#include "hash_reversal/probability.hpp"

namespace hash_reversal {

Probability::Probability(std::shared_ptr<Dataset> dataset,
												 std::shared_ptr<utils::Config> config) : dataset_(dataset), config_(config) {
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

double Probability::probOne(size_t rv_index,
														const std::vector<VariableAssignment> &observed_neighbors,
														const std::string &algorithm) const {
	// First check if the desired RV is already observed. If yes, we have our answer easy!
	const double eps = config_->epsilon;
	for (const auto &observed : observed_neighbors) {
		if (observed.rv_index == rv_index) {
			return observed.value ? 1.0 - eps : eps;
		}
	}

	double prob_one = -1;

	if (algorithm == "cpd") {
		std::vector<VariableAssignment> rv_assignments = observed_neighbors;
		rv_assignments.push_back(VariableAssignment(rv_index, 0));
    const size_t count0 = count(rv_assignments);
    rv_assignments.pop_back();
    rv_assignments.push_back(VariableAssignment(rv_index, 1));
    const size_t count1 = count(rv_assignments);
    const size_t total = count0 + count1;
		if (total > 0) prob_one = double(count1) / total;
	} else if (algorithm == "noisy-or") {
		spdlog::warn("Noisy-OR model does not work well as an approximation for this problem!");
		const double N = double(config_->num_train_samples);
		const auto rv_assignment = VariableAssignment(rv_index, 1);
		const double leak = 0.1;
		double product = 1.0 - leak;
		for (const auto &observed : observed_neighbors) {
			product *= 1.0 - count({rv_assignment, observed}) / N;
		}
		prob_one = 1.0 - product;
	} else if (algorithm == "auto-select") {
		size_t num_observed = observed_neighbors.size() + 1;
		while (prob_one == -1) {
			num_observed--;
			const auto subset = utils::Convenience::randomSubset(observed_neighbors, num_observed);
			prob_one = probOne(rv_index, subset, "cpd");
		}
		if (num_observed < observed_neighbors.size()) {
			spdlog::warn("Took subset of {}/{} observed neighbors",
									 num_observed, observed_neighbors.size());
		}
	} else {
		spdlog::error("Probability algorithm '{}' is not implemented!", algorithm);
	}

	return prob_one == -1 ? -1 : std::max(eps, std::min(1.0 - eps, prob_one));
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