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
                            const std::vector<VariableAssignment> &observed_neighbors) const {
  // First check if the desired RV is already observed. If yes, we have our answer easy!
  const double eps = config_->epsilon;
  for (const auto &observed : observed_neighbors) {
    if (observed.rv_index == factor.primary_rv) {
      return observed.value ? 1.0 - eps : eps;
    }
  }

  double prob_one = 0.5;

  /*
  std::vector<VariableAssignment> rv_assignments = observed_neighbors;
  rv_assignments.push_back(VariableAssignment(rv_index, 0));
  const size_t count0 = count(rv_assignments);
  rv_assignments.pop_back();
  rv_assignments.push_back(VariableAssignment(rv_index, 1));
  const size_t count1 = count(rv_assignments);
  const size_t total = count0 + count1;
  if (total > 0) prob_one = double(count1) / total;
  */

  return std::max(eps, std::min(1.0 - eps, prob_one));
}

}  // end namespace hash_reversal