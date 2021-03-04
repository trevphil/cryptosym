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

#include "bp/node.hpp"

namespace preimage {

namespace bp {

/***************************
 ******* GRAPH EDGE ********
 ***************************/

GraphEdge::GraphEdge(std::shared_ptr<GraphNode> n,
                     std::shared_ptr<GraphFactor> f,
                     IODirection dir, const std::vector<size_t> &node_is)
    : node(n), factor(f), direction(dir), node_indices(node_is) {}

std::string GraphEdge::toString() const {
  std::stringstream ss;
  ss << node->toString() << " <-> " << factor->toString();
  return ss.str();
}

/***************************
 ****** GRAPH FACTOR *******
 ***************************/

GraphFactor::GraphFactor(size_t i, FactorType t)
    : is_leaf_(false), index_(i), t_(t) {
  // TODO: is_leaf_ should not always be false
}

std::string GraphFactor::toString() const { return "Factor"; }

size_t GraphFactor::index() const { return index_; }

bool GraphFactor::isLeaf() const { return is_leaf_; }

FactorType GraphFactor::type() const { return t_; }

void GraphFactor::initMessages() {
  for (std::shared_ptr<GraphEdge> edge : edges_) {
    const size_t numbits = edge->cardinality;
    const size_t vals = (1 << numbits);
    edge->m2n = Eigen::VectorXd(vals) * (1.0 / vals);
  }
}

Eigen::MatrixXd GraphFactor::gatherIncoming() const {
  const size_t l = edges_.size();
  Eigen::MatrixXd msg_in = Eigen::MatrixXd::Zero(l, 2);
  for (size_t i = 0; i < l; ++i) {
    msg_in.row(i) = edges_.at(i)->m2f;
  }
  return msg_in;
}

void GraphFactor::factor2node() {
  spdlog::warn("This method should be implemented in child classes!");
}

void GraphFactor::addEdge(std::shared_ptr<GraphEdge> e) {
  edges_.push_back(e);
}

/***************************
 ******* GRAPH NODE ********
 ***************************/

GraphNode::GraphNode(size_t i) : index_(i) {}

std::string GraphNode::toString() const {
  std::stringstream ss;
  ss << "Node " << index_ << ": (" << final_dist_(0);
  ss << ", " << final_dist_(1) << ")";
  return ss.str();
}

size_t GraphNode::index() const { return index_; }

void GraphNode::initMessages() {
  for (std::shared_ptr<GraphEdge> edge : edges_) {
    edge->m2f = Eigen::Vector2d::Ones() * 0.5;
  }

  size_t n_in = 0, n_out = 0, n_prior = 0;
  for (IODirection direction : directions_) {
    switch (direction) {
    case IODirection::inp:
      n_in++;
      break;
    case IODirection::out:
      n_out++;
      break;
    case IODirection::prior:
      n_prior++;
      break;
    case IODirection::none:
      break;
    }
  }

  in_factors_ = Eigen::VectorXd::Zero(n_in);
  out_factors_ = Eigen::VectorXd::Zero(n_out);
  prior_factors_ = Eigen::VectorXd::Zero(n_prior);

  const size_t l = edges_.size();
  prev_msg_ = Eigen::MatrixXd::Ones(l, 2) * 0.5;
  prev_p0_ = Eigen::VectorXd::Ones(l) * 0.5;
  prev_p1_ = Eigen::VectorXd::Ones(l) * 0.5;
}

Eigen::MatrixXd GraphNode::gatherIncoming() const {
  const size_t m = prev_msg_.rows();
  assert(m == edges_.size());
  Eigen::MatrixXd msg_in = Eigen::MatrixXd::Zero(m, 2);
  for (size_t i = 0; i < m; ++i) {
    msg_in.row(i) = edges_.at(i)->m2n;
  }
  return msg_in;
}

void GraphNode::node2factor(IODirection target) {
  // TODO: target is not used yet
  const size_t l = edges_.size();
  const Eigen::MatrixXd msg = gatherIncoming();
  const double d = BP_DAMPING;

  // TODO: use .replicate(repeat_rows, repeat_cols)
  Eigen::MatrixXd tmp0 = Eigen::MatrixXd::Zero(l, msg.rows());
  for (size_t i = 0; i < l; ++i) tmp0.row(i) = msg.col(0);
  tmp0.diagonal().fill(1.0);
  Eigen::VectorXd p0 = tmp0.rowwise().prod();

  Eigen::MatrixXd tmp1 = Eigen::MatrixXd::Zero(l, msg.rows());
  for (size_t i = 0; i < l; ++i) tmp1.row(i) = msg.col(1);
  tmp1.diagonal().fill(1.0);
  Eigen::VectorXd p1 = tmp1.rowwise().prod();

  const Eigen::VectorXd s = p0 + p1;
  if ((s.array() == 0.0).any()) {
    spdlog::error("Node {}: zero-sum! Contradiction in factor graph?");
    assert(false);
  }

  p0 = p0.array() / s.array();
  p1 = p1.array() / s.array();
  p0 = (p0 * d) + (prev_p0_ * (1 - d));
  p1 = (p1 * d) + (prev_p1_ * (1 - d));

  for (size_t i = 0; i < edges_.size(); i++) {
    edges_.at(i)->m2f(0) = p0(i);
    edges_.at(i)->m2f(1) = p1(i);
  }

  inlineNorm(msg);
  prev_p0_ = p0;
  prev_p1_ = p1;
  prev_msg_ = msg;
}

void GraphNode::norm() {
  const Eigen::MatrixXd Mm = gatherIncoming();
  const Eigen::ArrayXd Zn = Mm.colwise().prod();
  const Eigen::ArrayXd P = Zn / Zn.sum();
  final_dist_ = P;
  change_ = fabs(final_dist_(0) - prev_dist_(0));
  prev_dist_ = final_dist_.replicate(1, 1);
  bit_ = final_dist_(1) > final_dist_(0) ? true : false;
}

void GraphNode::inlineNorm(const Eigen::MatrixXd &msg) {
  const Eigen::ArrayXd Zn = msg.colwise().prod();
  const Eigen::ArrayXd P = Zn / Zn.sum();
  final_dist_ = P;
  bit_ = final_dist_(1) > final_dist_(0) ? true : false;
}

void GraphNode::addEdge(std::shared_ptr<GraphEdge> e) {
  edges_.push_back(e);
}

}  // end namespace bp

}  // end namespace preimage
