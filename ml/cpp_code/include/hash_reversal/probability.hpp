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
#include <Eigen/Dense>
#include <boost/dynamic_bitset.hpp>

#include "hash_reversal/dataset.hpp"
#include "utils/config.hpp"
#include "utils/convenience.hpp"

namespace hash_reversal {

class Probability {
 public:
  struct VariableAssignment {
		VariableAssignment(size_t rv_idx, bool v) : rv_index(rv_idx), value(v) { }
		size_t rv_index;
		bool value;
	};

	explicit Probability(const Dataset &dataset, const utils::Config &config);

	size_t count(const std::vector<VariableAssignment> &rv_assignments) const;

 private:
  Dataset dataset_;
	Eigen::MatrixXd phats_;
};

}  // end namespace hash_reversal