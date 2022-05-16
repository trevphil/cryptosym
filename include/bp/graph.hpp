/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#pragma once

#include <map>
#include <memory>
#include <vector>

#include "bp/node.hpp"

namespace preimage {

namespace bp {

class Graph {
 public:
  Graph();

  virtual ~Graph();

  void printGraph() const;

  void writeNodes() const;

  void addFactor(std::shared_ptr<GraphFactor> factor);

  void addNode(std::shared_ptr<GraphNode> node);

  bool hasNode(int index) const;

  bool hasFactor(int index, BPFactorType t) const;

  std::shared_ptr<GraphNode> getNode(int index) const;

  std::shared_ptr<GraphFactor> getFactor(int index, BPFactorType t) const;

  double entropySum() const;

  double maxChange() const;

  void connectFactorNode(std::shared_ptr<GraphFactor> factor,
                         std::shared_ptr<GraphNode> node, IODirection dir,
                         bool negated = false);

  int iterations() const;

  void norm();

  void initMessages();

  void spreadPriors(const std::vector<int> &prior_rvs);

  void scheduledUpdate();

  std::vector<std::vector<std::shared_ptr<GraphNode>>> schedule_variable;
  std::vector<std::vector<std::shared_ptr<GraphFactor>>> schedule_factor;

 private:
  int iter_;
  std::vector<std::shared_ptr<GraphFactor>> factors_;
  std::map<std::string, std::shared_ptr<GraphFactor>> factor_map_;
  std::vector<std::shared_ptr<GraphNode>> nodes_;
  std::map<int, std::shared_ptr<GraphNode>> node_map_;
};

}  // end namespace bp

}  // end namespace preimage
