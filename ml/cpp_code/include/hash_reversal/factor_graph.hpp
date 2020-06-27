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
#include <Eigen/Dense>

#include "utils/config.hpp"
#include "utils/convenience.hpp"
#include "hash_reversal/probability.hpp"

namespace hash_reversal {

class FactorGraph {
 public:
  struct Prediction {
    double prob_bit_is_one;
    double log_likelihood_ratio;
  };

  explicit FactorGraph(const Probability &prob, const utils::Config &config);

  Prediction predict(size_t bit_index, bool bit_value, std::map<size_t, bool> observed);

 private:
  Eigen::MatrixXf adjacency_mat_;
};

}  // end namespace hash_reversal