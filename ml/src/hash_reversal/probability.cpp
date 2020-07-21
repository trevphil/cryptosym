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

Probability::Probability(std::shared_ptr<utils::Config> config) : config_(config) {
}

double Probability::probOne(const Factor &factor,
                            const VariableAssignments &assignments) const {
  const double eps = config_->epsilon;

  double output_prob_one = 0.5;
  const std::string &factor_type = factor.factor_type;
  const auto values = factor.extractInputOutput(assignments);

  if (factor_type == "AND") {
    spdlog::error("NOT IMPLEMENTED");

  } else if (factor_type == "XOR") {
    // TODO - Need to represent uncertaintly that 1 = (0 ^ 1) but also 1 = (1 ^ 0) ?
    output_prob_one = values.out == (values.in1 ^ values.in2);

  } else if (factor_type == "XOR_C0") {
    output_prob_one = values.out == values.in1;

  } else if (factor_type == "XOR_C1") {
    output_prob_one = values.out != values.in1;

  } else if (factor_type == "OR") {
    spdlog::error("NOT IMPLEMENTED");

  } else if (factor_type == "INV") {
    output_prob_one = values.out != values.in1;

  } else if (factor_type == "SHIFT") {
    output_prob_one = values.out == values.in1;

  } else if (factor_type == "PRIOR") {
    output_prob_one = 0.5;

  } else {
    spdlog::error("Unsupported factor: {}", factor_type);
  }

  return std::max(eps, std::min(1.0 - eps, output_prob_one));
}

}  // end namespace hash_reversal