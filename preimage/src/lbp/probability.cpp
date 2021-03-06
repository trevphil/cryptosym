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

#include <algorithm>

#include "lbp/probability.hpp"

#include <spdlog/spdlog.h>

namespace preimage {

namespace lbp {

Probability::Probability(const LbpParams &params) : params_(params) {}

double Probability::probOne(const LbpFactor &factor,
                            const std::map<size_t, bool> &assignments,
                            const std::map<size_t, bool> &observed) const {
  const double eps = params_.epsilon;
  double assignment_prob = 0.5;
  const auto values = factor.extract(assignments);
  const bool is_observed = observed.count(factor.output) > 0u;

  // should this be here?
  if (is_observed && values.out != observed.at(factor.output)) return eps;

  if (factor.t == Factor::Type::AndFactor) {
    if (values.out == 0) {
      assignment_prob = values.out == (values.in1 & values.in2); // ? 1.0 / 3.0 : 0;
    } else {
      assignment_prob = values.out == (values.in1 & values.in2);
    }

  } else if (factor.t == Factor::Type::XorFactor) {
    assignment_prob = values.out == (values.in1 ^ values.in2); // ? 0.5 : 0;

  } else if (factor.t == Factor::Type::NotFactor) {
    assignment_prob = (values.out != values.in1);

  } else if (factor.t == Factor::Type::SameFactor) {
    assignment_prob = (values.out == values.in1);

  } else if (factor.t == Factor::Type::PriorFactor) {
    if (is_observed) {
      assignment_prob = (values.out == observed.at(factor.output));
    } else {
      assignment_prob = 0.5;
    }

  } else {
    spdlog::error("Unsupported factor: {}", factor.toString());
  }

  return std::max(eps, assignment_prob);
}

}  // end namespace lbp

}  // end namespace preimage