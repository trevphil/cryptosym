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

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "hash_reversal/variable_assignments.hpp"
#include "utils/config.hpp"

namespace hash_reversal {

/*****************************************
 *********** FACTOR GRAPH NODE ***********
 *****************************************/

class FactorGraphNode {
 public:
  virtual ~FactorGraphNode();

  void updateMessage(size_t to, bool rv_val, double new_msg,
                     std::shared_ptr<utils::Config> config);

  double prevMessage(size_t to, bool rv_val) const;

  void reset(const VariableAssignments &observed = {},
             std::shared_ptr<utils::Config> config = nullptr);

 protected:
  virtual std::set<size_t> neighbors() const;

  std::map<std::pair<size_t, double>, double> messages_;
};

/*****************************************
 ************ RANDOM VARIABLE ************
 *****************************************/

class RandomVariable : public FactorGraphNode {
 public:
  std::set<size_t> factor_indices;

 private:
  std::set<size_t> neighbors() const override;
};

/*****************************************
 **************** FACTOR *****************
 *****************************************/

class Factor : public FactorGraphNode {
 public:
  struct Values {
    bool in1, in2, out;
  };

  Factor(const std::string &ftype, size_t out, const std::set<size_t> &ref);

  Values extract(const VariableAssignments &assignments) const;

  std::vector<size_t> inputRVs() const;

  std::string factor_type;
  size_t output_rv;
  std::set<size_t> referenced_rvs;

 private:
  std::set<size_t> neighbors() const override;
};

}  // end namespace hash_reversal