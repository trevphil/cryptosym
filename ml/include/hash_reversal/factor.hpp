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

#include <spdlog/spdlog.h>

#include <set>
#include <map>
#include <string>
#include <utility>
#include <memory>

#include "hash_reversal/variable_assignments.hpp"
#include "utils/config.hpp"

namespace hash_reversal {

class FactorGraphNode {
 public:
  void updateMessage(size_t to, bool rv_val, double new_msg,
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

  double prevMessage(size_t to, bool rv_val) const {
    const std::pair<size_t, bool> key = {to, rv_val};
    if (messages_.count(key) > 0) return messages_.at(key);
    return 1.0;
  }

  void reset() {
    messages_.clear();
  }

 protected:
  std::map<std::pair<size_t, double>, double> messages_;
};

class RandomVariable : public FactorGraphNode {
 public:
  std::set<size_t> factor_indices;
};

class Factor : public FactorGraphNode {
 public:
  struct Values {
    bool in1, in2, out;
  };

  Factor(size_t rv) : factor_type("PRIOR"), output_rv(rv) {
    referenced_rvs.insert(rv);
  }

  Values extractInputOutput(const VariableAssignments &assignments) const {
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

  std::string factor_type;
  size_t output_rv;
  std::set<size_t> referenced_rvs;
};

} // end namespace hash_reversal