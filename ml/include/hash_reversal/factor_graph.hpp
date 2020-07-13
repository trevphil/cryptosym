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

#include <Eigen/Dense>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "utils/config.hpp"
#include "utils/convenience.hpp"
#include "hash_reversal/factor.hpp"
#include "hash_reversal/probability.hpp"

namespace hash_reversal {

class FactorGraph {
 public:
  struct Prediction {
    double prob_bit_is_one;
    double log_likelihood_ratio;
  };

  explicit FactorGraph(std::shared_ptr<Probability> prob,
                       std::shared_ptr<utils::Config> config);

  void runLBP(const std::vector<VariableAssignment> &observed);

  Prediction predict(size_t rv_index) const;

  std::vector<Prediction> marginals() const;

 private:
  void setupFactors();
  void setupLBP(const std::vector<VariableAssignment> &observed);
  void updateFactorMessages();
  void updateRandomVariableMessages();
  void printConnections() const;
  bool equal(const std::vector<Prediction> &marginals1,
             const std::vector<Prediction> &marginals2,
             double tol = 1e-4) const;

  std::shared_ptr<Probability> prob_;
  std::shared_ptr<utils::Config> config_;
  std::vector<RandomVariable> rvs_;
  std::vector<Factor> factors_;
  Eigen::MatrixXd rv_messages_;
  Eigen::MatrixXd factor_messages_;
  Eigen::VectorXd rv_initialization_;
  Eigen::VectorXd factor_initialization_;
  std::vector<Prediction> previous_marginals_;
};

}  // end namespace hash_reversal