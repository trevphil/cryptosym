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

  virtual ~Graph();

  void printGraph() const;

  void addFactor(std::shared_ptr<GraphFactor> factor);

  void addNode(std::shared_ptr<GraphNode> node);

  bool hasNode(size_t index) const;

  bool hasFactor(size_t index, BPFactorType t) const;

  std::shared_ptr<GraphNode> getNode(size_t index) const;

  std::shared_ptr<GraphFactor> getFactor(size_t index, BPFactorType t) const;

  double entropySum() const;

  double maxChange() const;

  void connectFactorNode(std::shared_ptr<GraphFactor> factor,
                         std::shared_ptr<GraphNode> node,
                         IODirection dir);

  size_t iterations() const;

  void norm();

  void initMessages();

  void spreadPriors(const std::vector<size_t> &prior_rvs);

  void scheduledUpdate();

  std::vector<std::vector<std::shared_ptr<GraphNode>>> schedule_variable;
  std::vector<std::vector<std::shared_ptr<GraphFactor>>> schedule_factor;

 private:
  size_t iter_;
  std::vector<std::shared_ptr<GraphFactor>> factors_;
  std::map<std::string, std::shared_ptr<GraphFactor>> factor_map_;
  std::vector<std::shared_ptr<GraphNode>> nodes_;
  std::map<size_t, std::shared_ptr<GraphNode>> node_map_;
};

}  // end namespace bp

}  // end namespace preimage