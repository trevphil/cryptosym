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

#include <memory>
#include <vector>
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

  struct RandomVariable {
    std::vector<size_t> factor_indices;
  };

  struct Factor {
    std::vector<size_t> rv_indices;
  };

  explicit FactorGraph(std::shared_ptr<Probability> prob,
                       std::shared_ptr<utils::Config> config);

  Prediction predict(size_t bit_index,
                     const std::vector<VariableAssignment> &observed);

 private:
  void setupLBP(const std::vector<VariableAssignment> &observed);
  void runLBP();
  void printConnections() const;

  std::shared_ptr<Probability> prob_;
  std::shared_ptr<utils::Config> config_;
  std::vector<RandomVariable> rvs_;
  std::vector<Factor> factors_;
  Eigen::MatrixXd rv_messages_;
  Eigen::MatrixXd factor_messages_;
  Eigen::VectorXd rv_initialization_;
  Eigen::VectorXd factor_initialization_;
};

}  // end namespace hash_reversal