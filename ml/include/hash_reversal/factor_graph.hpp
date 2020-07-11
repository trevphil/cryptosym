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

#include <set>
#include <map>
#include <memory>
#include <vector>

#include "utils/config.hpp"
#include "utils/convenience.hpp"
#include "hash_reversal/probability.hpp"
#include "hash_reversal/dataset.hpp"

namespace hash_reversal {

class FactorGraph {
 public:
  struct Prediction {
    double prob_bit_is_one;
    double log_likelihood_ratio;
  };

  struct RandomVariable {
    std::set<size_t> factor_indices;
  };

  struct Factor {
    std::set<size_t> rv_indices;
  };

  explicit FactorGraph(std::shared_ptr<Probability> prob,
                       std::shared_ptr<Dataset> dataset,
                       std::shared_ptr<utils::Config> config);

  void runLBP(const std::vector<VariableAssignment> &observed);

  Prediction predict(size_t rv_index);

 private:
  void setupUndirectedGraph();
  void setupDirectedGraph();
  void setupFactors();
  void setupLBP(const std::vector<VariableAssignment> &observed);
  void updateFactorMessages();
  void updateRandomVariableMessages();
  void printConnections() const;

  std::shared_ptr<Probability> prob_;
  std::shared_ptr<Dataset> dataset_;
  std::shared_ptr<utils::Config> config_;
  std::vector<RandomVariable> rvs_;
  std::vector<Factor> factors_;
  Eigen::MatrixXd rv_messages_;
  Eigen::MatrixXd factor_messages_;
  Eigen::VectorXd rv_initialization_;
  Eigen::VectorXd factor_initialization_;

  std::map<size_t, std::set<size_t>> udg_;
  std::map<size_t, std::set<size_t>> dg_;
  std::map<size_t, std::set<size_t>> reversed_dg_;
};

}  // end namespace hash_reversal