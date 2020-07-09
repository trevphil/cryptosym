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

#pragma once

#include <vector>
#include <memory>
#include <Eigen/Dense>
#include <boost/dynamic_bitset.hpp>

#include "hash_reversal/dataset.hpp"
#include "hash_reversal/variable_assignment.hpp"
#include "utils/config.hpp"
#include "utils/convenience.hpp"

namespace hash_reversal {

class Probability {
 public:
	explicit Probability(std::shared_ptr<Dataset> dataset,
											 std::shared_ptr<utils::Config> config);

	double probOne(size_t rv_index,
								 const std::vector<VariableAssignment> &observed_neighbors,
								 const std::string &algorithm) const;

	size_t count(const std::vector<VariableAssignment> &rv_assignments) const;

 private:
  std::shared_ptr<Dataset> dataset_;
	std::shared_ptr<utils::Config> config_;
	Eigen::MatrixXd phats_;
};

}  // end namespace hash_reversal