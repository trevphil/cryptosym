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
#include <vector>
#include <utility>

#include "lbp/params.hpp"
#include "factor.hpp"

namespace preimage {

namespace lbp {

/*****************************************
 *********** FACTOR GRAPH NODE ***********
 *****************************************/

class FactorGraphNode {
 public:
  virtual ~FactorGraphNode();

  void updateMessage(size_t to, bool rv_val, double new_msg,
                     const LbpParams &config);

  double prevMessage(size_t to, bool rv_val) const;

  void clearMessages();

 protected:
  std::map<std::pair<size_t, bool>, double> messages_;
};

/*****************************************
 ************ RANDOM VARIABLE ************
 *****************************************/

class RandomVariable : public FactorGraphNode {
 public:
  std::vector<size_t> factor_indices;
};

/*****************************************
 **************** FACTOR *****************
 *****************************************/

class LbpFactor : public Factor, public FactorGraphNode {
 public:
  struct Values {
    bool in1, in2, out;
  };

  LbpFactor();

  explicit LbpFactor(const Factor &factor);

  Values extract(const std::map<size_t, bool> &assignments) const;

  std::vector<size_t> referenced_rvs;
};

}  // end namespace lbp

}  // end namespace preimage
