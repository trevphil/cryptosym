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
#include <string>
#include <iostream>
#include <memory>
#include <eigen3/Eigen/Eigen>

#include "bp/params.hpp"
#include "core/logic_gate.hpp"

namespace preimage {

namespace bp {

enum class IODirection : uint8_t {
  None = 0, Input = 1, Output = 2, Prior = 3
};

enum class BPFactorType : uint8_t {
  Prior = 0, And = 1, Xor = 2, Or = 3, Maj = 4, Xor3 = 5
};

class GraphNode;
class GraphFactor;

class GraphEdge {
 public:
  GraphEdge(std::shared_ptr<GraphNode> n,
            std::shared_ptr<GraphFactor> f,
            IODirection dir, bool neg);

  std::string toString() const;

  std::shared_ptr<GraphNode> node;
  std::shared_ptr<GraphFactor> factor;
  IODirection direction;
  bool negated;
  Eigen::Vector2d m2f;
  Eigen::Vector2d m2n;
};

class GraphFactor {
 public:
  GraphFactor(int i, BPFactorType t);

  virtual ~GraphFactor();

  static std::string ftype2str(BPFactorType t);

  static std::string makeString(int index, BPFactorType t);

  std::string toString() const;

  int index() const;

  BPFactorType type() const;

  std::vector<std::shared_ptr<GraphEdge>> edges() const;

  virtual void initMessages();

  Eigen::MatrixXd gatherIncoming() const;

  virtual void factor2node();

  void addEdge(std::shared_ptr<GraphEdge> e);

 protected:
  int index_;
  BPFactorType t_;
  std::vector<std::shared_ptr<GraphEdge>> edges_;
  Eigen::MatrixXd table_;

 private:
  std::map<int, int> edge_index_for_table_column_;
};

class GraphNode {
 public:
  explicit GraphNode(int i);

  virtual ~GraphNode();

  void rescaleMatrix(Eigen::MatrixXd &m);

  Eigen::Array2d stableColwiseProduct(const Eigen::MatrixXd &m);

  std::string toString() const;

  int index() const;

  bool bit() const;

  double entropy() const;

  double change() const;

  double distanceFromUndetermined() const;

  std::vector<std::shared_ptr<GraphEdge>> edges() const;

  void initMessages();

  Eigen::MatrixXd gatherIncoming() const;

  void node2factor(IODirection target = IODirection::None);

  void norm();

  void inlineNorm(const Eigen::MatrixXd &msg);

  void addEdge(std::shared_ptr<GraphEdge> e);

  static int num_resets;

 private:
  bool bit_;
  double entropy_;
  double change_;
  int index_;
  std::vector<std::shared_ptr<GraphEdge>> edges_;
  std::vector<IODirection> directions_;
  std::vector<int> in_factor_idx_;
  std::vector<int> out_factor_idx_;
  std::vector<int> all_factor_idx_;
  Eigen::MatrixXd prev_in_, prev_out_;
  Eigen::Vector2d prev_dist_, final_dist_;
};

}  // end namespace bp

}  // end namespace preimage
