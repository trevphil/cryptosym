/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include "bp/graph.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

namespace preimage {

namespace bp {

Graph::Graph() : iter_(0) {}

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
  printf("%s\n", "--------- GRAPH ---------");
  printf("%s\n", ">>> nodes <<<");

  for (auto &n : nodes_) {
    printf("%s\n", n->toString().c_str());
    for (auto &e : n->edges()) {
      printf("\t%s : m2f=[%f, %f]\n", e->toString().c_str(), e->m2f(0), e->m2f(1));
    }
  }
  printf("%s\n", ">>> factors <<<");
  for (auto &f : schedule_factor.at(0)) {
    printf("%s\n", f->toString().c_str());
    for (auto &e : f->edges()) {
      printf("\t%s : m2n=[%f, %f]\n", e->toString().c_str(), e->m2n(0), e->m2n(1));
    }
  }
  printf("%s\n", "---------------------");
}

void Graph::writeNodes() const {
  std::ofstream outfile;
  outfile.open("/tmp/bp_dist.txt", std::ios::out | std::ios::app);
  for (unsigned int i = 0; i < nodes_.size(); i++) {
    outfile << nodes_.at(i)->distanceFromUndetermined();
    if (i != nodes_.size() - 1) outfile << ",";
  }
  outfile << std::endl;
  outfile.close();

  outfile.open("/tmp/bp_bits.txt", std::ios::out | std::ios::app);
  for (unsigned int i = 0; i < nodes_.size(); i++) {
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

bool Graph::hasNode(int index) const { return node_map_.count(index) > 0; }

bool Graph::hasFactor(int index, BPFactorType t) const {
  const std::string s = GraphFactor::makeString(index, t);
  return factor_map_.count(s) > 0;
}

std::shared_ptr<GraphNode> Graph::getNode(int index) const { return node_map_.at(index); }

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
                              std::shared_ptr<GraphNode> node, IODirection dir,
                              bool negated) {
  std::shared_ptr<GraphEdge> e(new GraphEdge(node, factor, dir, negated));
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

  for (auto &node : schedule_variable.at(0)) {
    node->node2factor(IODirection::Input);
  }

  // ################# FORWARD  #################
  for (int r = 0; r < n_layers; r++) {
    for (auto &factor : schedule_factor.at(r)) {
      factor->factor2node();
    }
    for (auto &node : schedule_variable.at(r)) {
      node->node2factor(IODirection::Input);
    }
  }

  for (auto &node : schedule_variable.back()) {
    node->node2factor(IODirection::Output);
  }

  // ################# BACKWARD  #################
  for (int r = (int)n_layers - 1; r >= 0; r--) {
    for (int i = schedule_factor.at(r).size(); i-- > 0;) {
      schedule_factor.at(r).at(i)->factor2node();
    }
    for (int i = schedule_variable.at(r).size(); i-- > 0;) {
      schedule_variable.at(r).at(i)->node2factor(IODirection::Output);
    }
  }
}

}  // end namespace bp

}  // end namespace preimage
