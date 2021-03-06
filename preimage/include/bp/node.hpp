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
#include "factor.hpp"

namespace preimage {

namespace bp {

enum class IODirection : uint8_t {
  None = 0, Input = 1, Output = 2, Prior = 3
};

enum class FType : uint8_t {
  None = 0, Prior = 1, And = 2, Not = 3, Same = 4, Xor = 5
};

class GraphNode;
class GraphFactor;

class GraphEdge {
 public:
  GraphEdge(std::shared_ptr<GraphNode> n,
            std::shared_ptr<GraphFactor> f,
            IODirection dir);

  std::string toString() const;

  std::shared_ptr<GraphNode> node;
  std::shared_ptr<GraphFactor> factor;
  IODirection direction;
  Eigen::Vector2d m2f;
  Eigen::Vector2d m2n;
};

class GraphFactor {
 public:
  GraphFactor(size_t i, FType t);

  std::string toString() const;

  size_t index() const;

  FType type() const;

  void initMessages();

  Eigen::MatrixXd gatherIncoming() const;

  void factor2node();

  void addEdge(std::shared_ptr<GraphEdge> e);

 private:
  size_t index_;
  FType t_;
  std::vector<std::shared_ptr<GraphEdge>> edges_;
};

class GraphNode {
 public:
  explicit GraphNode(size_t i);

  std::string toString() const;

  size_t index() const;

  bool bit() const;

  double entropy() const;

  double change() const;

  void initMessages();

  Eigen::MatrixXd gatherIncoming() const;

  void node2factor(IODirection target = IODirection::None);

  void norm();

  void inlineNorm(const Eigen::MatrixXd &msg);

  void addEdge(std::shared_ptr<GraphEdge> e);

 private:
  bool bit_;
  double entropy_;
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
