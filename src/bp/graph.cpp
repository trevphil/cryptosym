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
#include <iostream>
#include <fstream>

#include <spdlog/spdlog.h>

#include "bp/graph.hpp"

namespace preimage {

namespace bp {

Graph::Graph() : iter_(0) {
  GraphNode::num_resets = 0;
}

Graph::~Graph() {
  schedule_factor.clear();
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

  nodes_.clear();
  node_map_.clear();
  factors_.clear();
  factor_map_.clear();
}

void Graph::printGraph() const {
  spdlog::info("--------- GRAPH ---------");
  spdlog::info(">>> nodes <<<");

  for (auto &n : nodes_) {
    spdlog::info("{}", n->toString());
    for (auto &e : n->edges()) {
      spdlog::info("\t{} : m2f=[{}, {}]", e->toString(),
                   e->m2f(0), e->m2f(1));
    }
  }
  spdlog::info(">>> factors <<<");
  for (auto &f : schedule_factor.at(0)) {
    spdlog::info("{}", f->toString());
    for (auto &e : f->edges()) {
      spdlog::info("\t{} : m2n=[{}, {}]", e->toString(),
                   e->m2n(0), e->m2n(1));
    }
  }
  spdlog::info("---------------------");
}

void Graph::writeNodes() const {
  std::ofstream outfile;
  outfile.open("/tmp/bp_dist.txt", std::ios::out | std::ios::app);
  for (int i = 0; i < nodes_.size(); i++) {
    outfile << nodes_.at(i)->distanceFromUndetermined();
    if (i != nodes_.size() - 1) outfile << ",";
  }
  outfile << std::endl;
  outfile.close();

  outfile.open("/tmp/bp_bits.txt", std::ios::out | std::ios::app);
  for (int i = 0; i < nodes_.size(); i++) {
    outfile << (int)nodes_.at(i)->bit();
    if (i != nodes_.size() - 1) outfile << ",";
  }
  outfile << std::endl;
  outfile.close();
}

void Graph::addFactor(std::shared_ptr<GraphFactor> factor) {
  factors_.push_back(factor);
  factor_map_[factor->toString()] = factor;
}

void Graph::addNode(std::shared_ptr<GraphNode> node) {
  nodes_.push_back(node);
  node_map_[node->index()] = node;
}

bool Graph::hasNode(int index) const {
  return node_map_.count(index) > 0;
}

bool Graph::hasFactor(int index, BPFactorType t) const {
  const std::string s = GraphFactor::makeString(index, t);
  return factor_map_.count(s) > 0;
}

std::shared_ptr<GraphNode> Graph::getNode(int index) const {
  return node_map_.at(index);
}

std::shared_ptr<GraphFactor> Graph::getFactor(int index, BPFactorType t) const {
  const std::string s = GraphFactor::makeString(index, t);
  return factor_map_.at(s);
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

int Graph::iterations() const { return iter_; }

void Graph::norm() {
  for (auto &node : nodes_) node->norm();
}

void Graph::initMessages() {
  for (auto &factor : factors_) factor->initMessages();
  for (auto &node : nodes_) node->initMessages();
}

void Graph::spreadPriors(const std::vector<int> &prior_rvs) {
  for (int rv : prior_rvs) {
    if (hasNode(rv)) getNode(rv)->node2factor();
  }
}

void Graph::scheduledUpdate() {
  iter_++;
  const int n_layers = schedule_factor.size();

#ifdef PRINT_DEBUG
  spdlog::info("A: starting scheduledUpdate()");
  printGraph();
#endif

  for (auto &node : schedule_variable.at(0)) {
    node->node2factor(IODirection::Input);
  }

#ifdef PRINT_DEBUG
  spdlog::info("B: scheduledUpdate() after node2factor[Input]");
  printGraph();
#endif

  // ################# FORWARD  #################
  for (int r = 0; r < n_layers; r++) {
    for (auto &factor : schedule_factor.at(r)) {
      factor->factor2node();
    }
    for (auto &node : schedule_variable.at(r)) {
      node->node2factor(IODirection::Input);
    }
  }

#ifdef PRINT_DEBUG
  spdlog::info("C: scheduledUpdate() after f2n --> n2f[Input]");
  printGraph();
#endif

  for (auto &node : schedule_variable.back()) {
    node->node2factor(IODirection::Output);
  }

#ifdef PRINT_DEBUG
  spdlog::info("D: scheduledUpdate() after node2factor[Output]");
  printGraph();
#endif

  // ################# BACKWARD  #################
  for (int r = (int)n_layers - 1; r >= 0; r--) {
    for (int i = schedule_factor.at(r).size(); i-- > 0;) {
      schedule_factor.at(r).at(i)->factor2node();
    }
    for (int i = schedule_variable.at(r).size(); i-- > 0;) {
      schedule_variable.at(r).at(i)->node2factor(IODirection::Output);
    }
  }
#ifdef PRINT_DEBUG
  spdlog::info("E: scheduledUpdate() after f2n --> n2f[Output]");
  printGraph();
#endif
}

}  // end namespace bp

}  // end namespace preimage
