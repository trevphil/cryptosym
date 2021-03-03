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

#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <eigen3/Eigen/Eigen>

#include "bp/params.hpp"

namespace preimage {

namespace bp {

enum IODirection : uint8_t {
  inp = 0, out = 1, prior = 2
};

class GraphNode;
class GraphFactor;

class GraphEdge {
 public:
  GraphEdge(const std::shared_ptr<GraphNode> &n,
            const std::shared_ptr<GraphFactor> &f,
            IODirection dir, const std::vector<size_t> &node_is);

  std::string toString() const;

  std::shared_ptr<GraphNode> node;
  std::shared_ptr<GraphFactor> factor;
  size_t cardinality;
  IODirection direction;
  std::vector<size_t> node_indices;
  Eigen::Vector2d m2f;
  Eigen::VectorXd m2n;  // TODO: can this be a Vector2d ?
};

class GraphFactor {
 public:
  GraphFactor();

  std::string toString() const;

  void initMessages();

  Eigen::MatrixXd gatherIncoming() const;

  void factor2node();

 private:
  bool is_leaf_;
  std::vector<std::shared_ptr<GraphEdge>> edges_;
};

class GraphNode {
 public:
  GraphNode(size_t node_index);

  std::string toString() const;

  void initMessages();

  Eigen::MatrixXd gatherIncoming() const;

  void node2factor();
  void norm();
  void inlineNorm(const Eigen::MatrixXd &msg);

 private:
  bool bit_;
  double change_;
  size_t index_;
  std::vector<std::shared_ptr<GraphEdge>> edges_;
  std::vector<IODirection> directions_;
  Eigen::VectorXd in_factors_, out_factors_, prior_factors_;
  Eigen::MatrixXd prev_msg_;
  Eigen::VectorXd prev_p0_, prev_p1_;
  Eigen::Vector2d prev_dist_, final_dist_;
};

}  // end namespace bp

}  // end namespace preimage
