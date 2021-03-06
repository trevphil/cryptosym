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

#include <algorithm>
#include <spdlog/spdlog.h>

#include "bp/graph.hpp"

namespace preimage {

namespace bp {

Graph::Graph() {}

Graph::~Graph() {
  schedule_factor.clear();
  schedule_prior.clear();
  schedule_variable.clear();

  for (auto &f : factors_) {
    if (f) f.reset();
  }
  for (auto &itr : factor_map_) {
    if (itr.second) itr.second.reset();
  }
  for (auto &n : nodes_) {
    if (n) n.reset();
  }
  for (auto &itr : node_map_) {
    if (itr.second) itr.second.reset();
  }
}

void Graph::addFactor(std::shared_ptr<GraphFactor> factor) {
  factors_.push_back(factor);
  factor_map_[factor->index()] = factor;
}

void Graph::addNode(std::shared_ptr<GraphNode> node) {
  nodes_.push_back(node);
  node_map_[node->index()] = node;
}

bool Graph::hasNode(size_t index) const {
  return node_map_.count(index) > 0;
}

bool Graph::hasFactor(size_t index) const {
  return factor_map_.count(index) > 0;
}

std::shared_ptr<GraphNode> Graph::getNode(size_t index) const {
  return node_map_.at(index);
}

std::shared_ptr<GraphFactor> Graph::getFactor(size_t index) const {
  return factor_map_.at(index);
}

double Graph::entropySum() const {
  double e = 0.0;
  for (auto &n : nodes_) e += n->entropy();
  return e;
}

double Graph::maxChange() const {
  double max_c = 0.0;
  for (auto &n : nodes_) max_c = std::max(max_c, n->change());
  return max_c;
}

void Graph::connectFactorNode(std::shared_ptr<GraphFactor> factor,
                              std::shared_ptr<GraphNode> node,
                              IODirection dir) {
  std::shared_ptr<GraphEdge> e(new GraphEdge(node, factor, dir));
  factor->addEdge(e);
  node->addEdge(e);
}

size_t Graph::iterations() const { return iter_; }

void Graph::norm() {
  for (auto &node : nodes_) node->norm();
}

void Graph::initMessages() {
  for (auto &factor : factors_) factor->initMessages();
  for (auto &node : nodes_) node->initMessages();
}

void Graph::scheduledUpdate() {
  iter_++;
  const size_t n_layers = schedule_factor.size();

  // Variables in the first layer are treated separately
  for (auto &node : schedule_variable.at(0)) {
    node->node2factor(IODirection::Input);
  }

  // ################# FORWARD  #################
  for (size_t r = 0; r < n_layers; r++) {
    const bool has_priors = (schedule_prior.at(r).size() > 0);
    // const bool has_priors = (schedule_prior.at(r + 1).size() > 0);
    for (auto &factor : schedule_factor.at(r)) {
      factor->factor2node();
    }

    for (auto &node : schedule_variable.at(r)) {
    // for (auto &node : schedule_variable.at(r + 1)) {
      if (has_priors) {
        node->node2factor(IODirection::Prior);
      } else {
        node->node2factor(IODirection::Input);
      }
    }

    if (has_priors) {
      for (auto &factor : schedule_prior.at(r)) {
      // for (auto &factor : schedule_prior.at(r + 1)) {
        factor->factor2node();
      }
      for (auto &node : schedule_variable.at(r)) {
      // for (auto &node : schedule_variable.at(r + 1)) {
        node->node2factor(IODirection::Input);
      }
    }
  }

  for (auto &node : schedule_variable.back()) {
    node->node2factor(IODirection::Output);
  }

  // ################# BACKWARD  #################
  for (int r = (int)n_layers - 1; r >= 0; r--) {
    const bool has_priors = (schedule_prior.at(r).size() > 0);
    for (auto &factor : schedule_factor.at(r)) {
      factor->factor2node();
    }

    for (auto &node : schedule_variable.at(r)) {
      if (has_priors) {
        node->node2factor(IODirection::Prior);
      } else {
        node->node2factor(IODirection::Output);
      }
    }

    if (has_priors) {
      for (auto &factor : schedule_prior.at(r)) {
        factor->factor2node();
      }
      for (auto &node : schedule_variable.at(r)) {
        node->node2factor(IODirection::Output);
      }
    }
  }
}

void Graph::spreadPriors() {
  for (auto &factor : factors_) {
    if (factor->type() == FType::Prior) {
      factor->factor2node();
    }
  }

  for (auto &factor : schedule_prior.at(0)) {
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
