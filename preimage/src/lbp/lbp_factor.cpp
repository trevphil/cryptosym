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

#include "lbp/lbp_factor.hpp"

namespace preimage {

namespace lbp {

/*****************************************
 *********** FACTOR GRAPH NODE ***********
 *****************************************/

FactorGraphNode::~FactorGraphNode() {}

void FactorGraphNode::updateMessage(size_t to, bool rv_val, double new_msg,
                                    const LbpParams &params) {
  const double damping = params.damping;
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

void FactorGraphNode::clearMessages() {
  messages_.clear();
}

/*****************************************
 **************** FACTOR *****************
 *****************************************/

LbpFactor::LbpFactor() : Factor() {}

LbpFactor::LbpFactor(const Factor &factor)
    : Factor(factor.t, factor.output, factor.inputs) {
  referenced_rvs = {output};
  for (size_t inp : inputs) referenced_rvs.push_back(inp);
}

LbpFactor::Values LbpFactor::extract(const std::map<size_t, bool> &assignments) const {
  Values v;
  bool did_set_in1 = false;
  for (size_t rv_index : referenced_rvs) {
    if (rv_index == output) {
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

}  // end namespace lbp

}  // end namespace preimage