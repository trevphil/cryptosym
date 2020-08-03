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

  double assignment_prob = 0.5;
  const std::string &factor_type = factor.factor_type;
  const auto values = factor.extractInputOutput(assignments);

  if (factor_type == "AND") {
    if (values.out == 0) {
      assignment_prob = values.out == (values.in1 & values.in2) ? 1.0/3.0 : 0;
    } else {
      assignment_prob = values.out == (values.in1 & values.in2);
    }

  } else if (factor_type == "AND_C0") {
    assignment_prob = values.out == 0 ? 0.5 : 0;

  } else if (factor_type == "AND_C1") {
    assignment_prob = values.out == values.in1;

  } else if (factor_type == "XOR") {
    assignment_prob = values.out == (values.in1 ^ values.in2) ? 0.5 : 0;

  } else if (factor_type == "XOR_C0") {
    assignment_prob = values.out == values.in1;

  } else if (factor_type == "XOR_C1") {
    assignment_prob = values.out != values.in1;

  } else if (factor_type == "OR") {
    if (values.out == 0) {
      assignment_prob = values.out == (values.in1 | values.in2);
    } else {
      assignment_prob = values.out == (values.in1 | values.in2) ? 1.0/3.0 : 0;
    }

  } else if (factor_type == "OR_C0") {
    assignment_prob = values.out == values.in1;

  } else if (factor_type == "OR_C1") {
    assignment_prob = values.out ? 0.5 : 0;

  } else if (factor_type == "INV") {
    assignment_prob = values.out != values.in1;

  } else if (factor_type == "ADD") {
    const int out = int(values.in1) + int(values.in2) + int(values.in3);
    assignment_prob = values.out == bool(out & 1) ? 0.25 : 0;

  } else if (factor_type == "ADD_C0") {
    const int sum = int(values.in1) + int(values.in2);
    assignment_prob = values.out == bool(sum & 1) ? 0.5 : 0;

  } else if (factor_type == "ADD_C1") {
    const int sum = int(values.in1) + int(values.in2);
    assignment_prob = values.out != bool(sum & 1) ? 0.5 : 0;

  } else if (factor_type == "ADD_C00") {
    assignment_prob = values.out == values.in1;

  } else if (factor_type == "ADD_C01") {
    assignment_prob = values.out != values.in1;

  } else if (factor_type == "ADD_C11") {
    assignment_prob = values.out == values.in1;

  } else if (factor_type == "ADD_CARRY") {
    const int out = int(values.in1) + int(values.in2) + int(values.in3);
    assignment_prob = values.out == bool((out >> 1) & 1) ? 0.25 : 0;

  } else if (factor_type == "ADD_CARRY_C0") {
    const bool carry = ((int(values.in1) + int(values.in2)) >> 1) & 1;
    if (values.out == 0) {
      // The two unknown inputs could be (0, 0), (0, 1), or (1, 0) for carry=0
      assignment_prob = values.out == carry ? 1.0/3.0 : 0;
    } else {
      // The two unknown inputs can only be (1, 1) for carry to be = 1
      assignment_prob = values.out == carry ? 1.0 : 0;
    }

  } else if (factor_type == "ADD_CARRY_C1") {
    const bool carry = ((int(values.in1) + int(values.in2)) >> 1) & 1;
    if (values.out == 0) {
      // Both unknown inputs must have been (0, 0) for carry to be = 0
      assignment_prob = values.out == carry ? 1.0 : 0;
    } else {
      // Unknown inputs could be (0, 1), (1, 0), or (1, 1) for carry=1
      assignment_prob = values.out == carry ? 1.0/3.0 : 0;
    }

  } else if (factor_type == "ADD_CARRY_C01") {
    assignment_prob = values.out == values.in1;

  } else if (factor_type == "PRIOR") {
    assignment_prob = 0.5;

  } else {
    spdlog::error("Unsupported factor: {}", factor_type);
  }

  return std::max(eps, std::min(1.0 - eps, assignment_prob));
}

}  // end namespace hash_reversal