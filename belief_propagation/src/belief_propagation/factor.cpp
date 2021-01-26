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

#include "belief_propagation/factor.hpp"

#include <spdlog/spdlog.h>

namespace belief_propagation {

/*****************************************
 *********** FACTOR GRAPH NODE ***********
 *****************************************/

FactorGraphNode::~FactorGraphNode() {}

void FactorGraphNode::updateMessage(size_t to, bool rv_val, double new_msg,
                                    std::shared_ptr<utils::Config> config) {
  const double damping = config->lbp_damping;

  const std::pair<size_t, bool> key = {to, rv_val};
  if (messages_.count(key) > 0) {
    const double prev_msg = messages_.at(key);
    messages_[key] = damping * new_msg + (1.0 - damping) * prev_msg;
  } else {
    messages_[key] = new_msg;
  }
}

double FactorGraphNode::prevMessage(size_t to, bool rv_val) const {
  const std::pair<size_t, bool> key = {to, rv_val};
  if (messages_.count(key) > 0) return messages_.at(key);
  return 1.0;
}

void FactorGraphNode::reset() {
  messages_.clear();
}

/*****************************************
 ************ RANDOM VARIABLE ************
 *****************************************/

/*****************************************
 **************** FACTOR *****************
 *****************************************/

Factor::Factor() : factor_type("NULL") {}

Factor::Factor(const std::string &ftype, size_t out, const std::set<size_t> &ref)
    : factor_type(ftype), output_rv(out), referenced_rvs(ref) {}

Factor::Values Factor::extract(const VariableAssignments &assignments) const {
  Values v;
  bool did_set_in1 = false;
  for (size_t rv_index : referenced_rvs) {
    if (rv_index == output_rv) {
      v.out = assignments.at(rv_index);
    } else if (!did_set_in1) {
      v.in1 = assignments.at(rv_index);
      did_set_in1 = true;
    } else {
      v.in2 = assignments.at(rv_index);
    }
  }
  return v;
}

std::vector<size_t> Factor::inputRVs() const {
  std::vector<size_t> inputs;
  for (size_t rv : referenced_rvs) {
    if (rv != output_rv) inputs.push_back(rv);
  }
  return inputs;
}

}  // end namespace belief_propagation