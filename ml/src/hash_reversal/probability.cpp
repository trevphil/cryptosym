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

#include "hash_reversal/probability.hpp"

namespace hash_reversal {

Probability::Probability(std::shared_ptr<utils::Config> config) : config_(config) {}

double Probability::probOne(const Factor &factor, const VariableAssignments &assignments,
                            const VariableAssignments &observed) const {
  const double eps = config_->epsilon;

  double assignment_prob = 0.5;
  const std::string &factor_type = factor.factor_type;
  const auto values = factor.extract(assignments);

  if (factor_type == "AND") {
    if (values.out == 0) {
      assignment_prob = values.out == (values.in1 & values.in2) ? 1.0 / 3.0 : 0;
    } else {
      assignment_prob = values.out == (values.in1 & values.in2);
    }

  } else if (factor_type == "SAME") {
    assignment_prob = values.out == values.in1;

  } else if (factor_type == "INV") {
    assignment_prob = values.out != values.in1;

  } else if (factor_type == "PRIOR") {
    if (observed.count(factor.output_rv) > 0u) {
      assignment_prob = observed.at(factor.output_rv) ? 1.0 : 0.0;
    } else {
      assignment_prob = 0.5;
    }

  } else {
    spdlog::error("Unsupported factor: {}", factor_type);
  }

  return std::max(eps, std::min(1.0 - eps, assignment_prob));
}

}  // end namespace hash_reversal