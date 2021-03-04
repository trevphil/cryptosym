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

#include <spdlog/spdlog.h>

#include "bp/graph.hpp"

namespace preimage {

namespace bp {

Graph::Graph() {}

void Graph::addFactor(std::shared_ptr<GraphFactor> factor) {
  factors_.push_back(factor);
  factor_map_[factor->index()] = factor;
}

void Graph::addNode(std::shared_ptr<GraphNode> node) {
  nodes_.push_back(node);
  node_map_[node->index()] = node;
}

void Graph::connectFactorNode(size_t fi, size_t ni, IODirection dir,
                              const std::vector<size_t> &node_indices) {
  std::shared_ptr<GraphFactor> target_factor = factor_map_.at(fi);
  std::shared_ptr<GraphNode> target_node = node_map_.at(ni);
  std::shared_ptr<GraphEdge> e(new GraphEdge(target_node,
      target_factor, dir, node_indices));
  target_factor->addEdge(e);
  target_node->addEdge(e);
}

void Graph::norm() {
  for (auto &node : nodes_) node->norm();
}

void Graph::factor2node() {
  for (auto &factor : factors_) factor->factor2node();
}

void Graph::node2factor() {
  for (auto &node : nodes_) node->node2factor();
}

void Graph::f2n2f() {
  // TODO: is this used?
  factor2node();
  node2factor();
  iter_++;
}

void Graph::n2f2n() {
  // TODO: is this used?
  node2factor();
  factor2node();
  iter_++;
}

void Graph::initMessages() {
  for (auto &factor : factors_) factor->initMessages();
  for (auto &node : nodes_) node->initMessages();
}

void Graph::scheduledUpdate() {
  iter_++;
  const size_t n_layers = schedule_factor_.size();

  // Variables in the first layer are treated separately
  for (auto &node : schedule_variable_.at(0)) {
    node->node2factor(IODirection::inp);
  }

  // ################# FORWARD  #################
  for (size_t r = 0; r < n_layers; r++) {
    const bool has_priors = (schedule_prior_.at(r + 1).size() > 0);
    for (auto &factor : schedule_factor_.at(r)) {
      factor->factor2node();
    }

    for (auto &node : schedule_variable_.at(r + 1)) {
      if (has_priors) {
        node->node2factor(IODirection::prior);
      } else {
        node->node2factor(IODirection::inp);
      }
    }

    if (has_priors) {
      for (auto &factor : schedule_prior_.at(r + 1)) {
        factor->factor2node();
      }
      for (auto &node : schedule_variable_.at(r + 1)) {
        node->node2factor(IODirection::inp);
      }
    }
  }

  for (auto &node : schedule_variable_.back()) {
    node->node2factor(IODirection::out);
  }

  // ################# BACKWARD  #################
  for (int r = n_layers - 1; r >= 0; r--) {
    const bool has_priors = (schedule_prior_.at(r).size() > 0);
    for (auto &factor : schedule_factor_.at(r)) {
      factor->factor2node();
    }

    for (auto &node : schedule_variable_.at(r)) {
      if (has_priors) {
        node->node2factor(IODirection::prior);
      } else {
        node->node2factor(IODirection::out);
      }
    }

    if (has_priors) {
      for (auto &factor : schedule_prior_.at(r)) {
        factor->factor2node();
      }
      for (auto &node : schedule_variable_.at(r)) {
        node->node2factor(IODirection::out);
      }
    }
  }
}

void Graph::spreadPriors() {
  const size_t n = schedule_prior_.size();
  for (size_t i = 0; i < n; i++) {
    // Filter out all factors which are leafs from the schedule
    std::vector<std::shared_ptr<GraphFactor>> tmp;
    for (auto &factor : schedule_prior_.at(i)) {
      if (!factor->isLeaf()) tmp.push_back(factor);
    }
    schedule_prior_[i] = tmp;
  }

  for (auto &factor : factors_) {
    if (factor->type() == FactorType::priorBit) {
      factor->factor2node();
    }
  }

  for (auto &factor : schedule_prior_.at(0)) {
    factor->factor2node();
  }

  /*
  TODO: important?
  for (auto &node : filter_nodes_t_.at(0)) {
    node->node2factor();
    node->norm();
  }
  */
}

}  // end namespace bp

}  // end namespace preimage
