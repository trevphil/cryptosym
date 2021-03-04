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
#include <map>
#include <memory>

#include "bp/node.hpp"

namespace preimage {

namespace bp {

class Graph {
 public:
  Graph();

  void addFactor(std::shared_ptr<GraphFactor> factor);

  void addNode(std::shared_ptr<GraphNode> node);

  void connectFactorNode(size_t fi, size_t ni, IODirection dir,
                         const std::vector<size_t> &node_indices = {});

  void norm();

  void factor2node();

  void node2factor();

  void f2n2f();

  void n2f2n();

  void initMessages();

  void scheduledUpdate();

  void spreadPriors();

 private:
  size_t iter_;
  std::vector<std::shared_ptr<GraphFactor>> factors_;
  // TODO: are the map variables actually used?
  std::map<size_t, std::shared_ptr<GraphFactor>> factor_map_;
  std::vector<std::shared_ptr<GraphNode>> nodes_;
  std::map<size_t, std::shared_ptr<GraphNode>> node_map_;
  std::vector<std::vector<std::shared_ptr<GraphNode>>> schedule_variable_;
  std::vector<std::vector<std::shared_ptr<GraphFactor>>> schedule_factor_;
  std::vector<std::vector<std::shared_ptr<GraphFactor>>> schedule_prior_;
};

}  // end namespace bp

}  // end namespace preimage