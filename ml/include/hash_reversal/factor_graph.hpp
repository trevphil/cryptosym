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
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphml.hpp>

#include <memory>
#include <vector>

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

  void runLBP(const std::vector<VariableAssignment> &observed);

  Prediction predict(size_t bit_index);

 private:
  void setupLBP(const std::vector<VariableAssignment> &observed);
  void printConnections() const;
  void saveGraphViz();

  size_t graph_viz_counter_;
  std::shared_ptr<Probability> prob_;
  std::shared_ptr<utils::Config> config_;
  std::vector<RandomVariable> rvs_;
  std::vector<Factor> factors_;
  Eigen::MatrixXd rv_messages_;
  Eigen::MatrixXd factor_messages_;
  Eigen::VectorXd rv_initialization_;
  Eigen::VectorXd factor_initialization_;

  struct VertexInfo {
    double weight;
  };

  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                VertexInfo> UndirectedGraph;
  UndirectedGraph udg_;
};

}  // end namespace hash_reversal