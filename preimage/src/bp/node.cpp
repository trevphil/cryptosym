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

#include <math.h>
#include <assert.h>
#include <algorithm>

#include <spdlog/spdlog.h>

#include "bp/node.hpp"

namespace preimage {

namespace bp {

/***************************
 ******* GRAPH EDGE ********
 ***************************/

GraphEdge::GraphEdge(std::shared_ptr<GraphNode> n,
                     std::shared_ptr<GraphFactor> f,
                     IODirection dir)
    : node(n), factor(f), direction(dir) {}

std::string GraphEdge::toString() const {
  std::stringstream ss;
  ss << node->toString() << " <-> " << factor->toString();
  return ss.str();
}

/***************************
 ****** GRAPH FACTOR *******
 ***************************/

GraphFactor::GraphFactor(size_t i, BPFactorType t)
    : index_(i), t_(t) {
  switch (t_) {
  case BPFactorType::None:
    assert(false);  // This should never happen
  case BPFactorType::Prior:
    break;
  case BPFactorType::Not:
    table_ = Eigen::MatrixXd::Zero(4, 3);
    // P(output = B | input = A)
    //        A  B  prob
    table_ << 0, 0, 0.0,
              0, 1, 1.0,
              1, 0, 1.0,
              1, 1, 0.0;
    break;
  case BPFactorType::Same:
    table_ = Eigen::MatrixXd::Zero(4, 3);
    // P(output = B | input = A)
    //        A  B  prob
    table_ << 0, 0, 1.0,
              0, 1, 0.0,
              1, 0, 0.0,
              1, 1, 1.0;
    break;
  case BPFactorType::And:
    table_ = Eigen::MatrixXd::Zero(8, 4);
    // P(output = C | input1 = A, input2 = B)
    //        A  B  C  prob
    table_ << 0, 0, 0, 1.0,
              0, 0, 1, 0.0,
              0, 1, 0, 1.0,
              0, 1, 1, 0.0,
              1, 0, 0, 1.0,
              1, 0, 1, 0.0,
              1, 1, 0, 0.0,
              1, 1, 1, 1.0;
    break;
  case BPFactorType::Xor:
    table_ = Eigen::MatrixXd::Zero(8, 4);
    // P(output = C | input1 = A, input2 = B)
    //        A  B  C  prob
    table_ << 0, 0, 0, 1.0,
              0, 0, 1, 0.0,
              0, 1, 0, 0.0,
              0, 1, 1, 1.0,
              1, 0, 0, 0.0,
              1, 0, 1, 1.0,
              1, 1, 0, 1.0,
              1, 1, 1, 0.0;
    break;
  }
}

GraphFactor::~GraphFactor() {
  for (auto &e : edges_) e.reset();
}

std::string GraphFactor::ftype2str(BPFactorType t) {
  switch (t) {
  case BPFactorType::None: return "None";
  case BPFactorType::And: return "And";
  case BPFactorType::Not: return "Not";
  case BPFactorType::Prior: return "Prior";
  case BPFactorType::Same: return "Same";
  case BPFactorType::Xor: return "Xor";
  }
}

std::string GraphFactor::makeString(size_t index, BPFactorType t) {
  std::stringstream ss;
  ss << "Factor " << index << " " << ftype2str(t);
  return ss.str();
}

std::string GraphFactor::toString() const {
  return GraphFactor::makeString(index_, t_);
}

size_t GraphFactor::index() const { return index_; }

BPFactorType GraphFactor::type() const { return t_; }

void GraphFactor::initMessages() {
  for (std::shared_ptr<GraphEdge> edge : edges_) {
    edge->m2n = Eigen::Vector2d::Ones() * 0.5;
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
  assert(t_ != BPFactorType::None);
  assert(t_ != BPFactorType::Prior);
  const size_t l = edges_.size();
  const Eigen::MatrixXd msg_in = gatherIncoming();
  assert(l == msg_in.rows());
  assert(l == table_.cols() - 1);

  Eigen::MatrixXd tfill = table_.replicate(1, 1);
  const size_t n_rows = tfill.rows();

  for (size_t node_idx = 0; node_idx < l; node_idx++) {
    Eigen::Array2d m = msg_in.row(node_idx);
    for (size_t row = 0; row < n_rows; row++) {
      if (table_(row, node_idx) == 0) {
        tfill(row, node_idx) = m(0);
      } else {
        tfill(row, node_idx) = m(1);
      }
    }
  }

  for (size_t i = 0; i < l; i++) {
    Eigen::MatrixXd tmp = tfill.replicate(1, 1);
    // Remove column "i" from tmp
    tmp.block(0, i, n_rows, l - i) = tmp.block(0, i + 1, n_rows, l - i);
    tmp.conservativeResize(n_rows, l);
    const Eigen::VectorXd p = tmp.rowwise().prod();
    double s0 = 0;
    double s1 = 0;
    for (size_t row = 0; row < table_.rows(); row++) {
      if (table_(row, i) == 0) {
        s0 += p(row);
      } else {
        s1 += p(row);
      }
    }
    edges_.at(i)->m2n(0) = s0;
    edges_.at(i)->m2n(1) = s1;
  }
}

void GraphFactor::addEdge(std::shared_ptr<GraphEdge> e) {
  edges_.push_back(e);
}

/***************************
 ******* GRAPH NODE ********
 ***************************/

GraphNode::GraphNode(size_t i) : index_(i) {}

GraphNode::~GraphNode() {
  for (auto &e : edges_) e.reset();
}

std::string GraphNode::toString() const {
  std::stringstream ss;
  ss << "Node " << index_ << ": (" << final_dist_(0);
  ss << ", " << final_dist_(1) << ")";
  return ss.str();
}

size_t GraphNode::index() const { return index_; }

bool GraphNode::bit() const { return bit_; }

double GraphNode::entropy() const { return entropy_; }

double GraphNode::change() const { return change_; }

void GraphNode::initMessages() {
  directions_ = {};
  for (std::shared_ptr<GraphEdge> edge : edges_) {
    edge->m2f = Eigen::Vector2d::Ones() * 0.5;
    directions_.push_back(edge->direction);
  }

  in_factor_idx_ = {};
  out_factor_idx_ = {};
  prior_factor_idx_ = {};
  all_factor_idx_ = {};
  for (size_t i = 0; i < directions_.size(); i++) {
    all_factor_idx_.push_back(i);
    switch (directions_.at(i)) {
    case IODirection::Input:
      in_factor_idx_.push_back(i);
      break;
    case IODirection::Output:
      out_factor_idx_.push_back(i);
      break;
    case IODirection::Prior:
      prior_factor_idx_.push_back(i);
      break;
    case IODirection::None:
      assert(false); // This should not happen
      break;
    }
  }

  change_ = 0.0;
  entropy_ = 0.0;
  is_first_msg_ = true;
  prev_dist_ = Eigen::Vector2d::Ones() * 0.5;
  final_dist_ = Eigen::Vector2d::Ones() * 0.5;

  const size_t l = edges_.size();
  prev_in_ = Eigen::MatrixXd::Ones(l, 2) * 0.5;
  prev_out_ = Eigen::MatrixXd::Ones(l, 2) * 0.5;
}

Eigen::MatrixXd GraphNode::gatherIncoming() const {
  const size_t l = edges_.size();
  Eigen::MatrixXd msg_in = Eigen::MatrixXd::Zero(l, 2);
  for (size_t i = 0; i < l; ++i) {
    // Clip to be >= 0 in case messages behavior is funny
    msg_in(i, 0) = std::max(0.0, edges_.at(i)->m2n(0));
    msg_in(i, 1) = std::max(0.0, edges_.at(i)->m2n(1));
  }
  return msg_in;
}

void GraphNode::node2factor(IODirection target) {
  const size_t l = edges_.size();
  Eigen::MatrixXd msg_in = gatherIncoming();
  const double d = BP_DAMPING;
  if (!is_first_msg_) {
    // Apply damping to the input
    msg_in = (msg_in * d) + (prev_in_ * (1 - d));
  }
  Eigen::MatrixXd msg_out = Eigen::MatrixXd::Zero(l, 2);

  std::vector<size_t> targets;
  switch (target) {
  case IODirection::None:
    targets = all_factor_idx_;
    break;
  case IODirection::Input:
    targets = in_factor_idx_;
    break;
  case IODirection::Output:
    targets = out_factor_idx_;
    break;
  case IODirection::Prior:
    targets = prior_factor_idx_;
    break;
  }

  for (size_t i : targets) {
    Eigen::MatrixXd tmp = msg_in.replicate(1, 1);
    // Remove row "i"
    tmp.block(i, 0, l - i - 1, 2) = tmp.block(i + 1, 0, l - i - 1, 2);
    tmp.conservativeResize(l - 1, 2);
    const Eigen::ArrayXd p = tmp.colwise().prod();
    const double s = p.sum();
    if (s == 0.0) {
      spdlog::error("Node {}: zero-sum! Is there a contradiction in the graph?");
      assert(false);
    }
    msg_out.row(i) = p / s;
  }

  if (!is_first_msg_) {
    // Apply damping to the output
    for (size_t i : targets) {
      msg_out.row(i) = (msg_out.row(i) * d) + (prev_out_.row(i) * (1 - d));
    }
  }

  // Propagate to the edges
  for (size_t i = 0; i < l; i++) {
    edges_.at(i)->m2f = msg_out.row(i);
  }

  inlineNorm(msg_in);
  prev_out_ = msg_out;
  prev_in_ = msg_in;
  is_first_msg_ = false;
}

void GraphNode::norm() {
  const Eigen::MatrixXd Mm = gatherIncoming();
  const Eigen::ArrayXd Zn = Mm.colwise().prod();
  const Eigen::ArrayXd P = Zn / Zn.sum();
  final_dist_ = P;
  double e = 0.0;
  for (size_t i = 0; i < P.cols(); i++) {
    if (P(i) != 0) e += -P(i) * log2(P(i));
  }
  entropy_ = e > 0 ? e : 0.0;
  change_ = fabs(final_dist_(0) - prev_dist_(0));
  prev_dist_ = final_dist_.replicate(1, 1);
  bit_ = final_dist_(1) > final_dist_(0) ? true : false;
}

void GraphNode::inlineNorm(const Eigen::MatrixXd &msg) {
  const Eigen::ArrayXd Zn = msg.colwise().prod();
  const Eigen::ArrayXd P = Zn / Zn.sum();
  final_dist_ = P;
  double e = 0.0;
  for (size_t i = 0; i < P.cols(); i++) {
    if (P(i) != 0) e += -P(i) * log2(P(i));
  }
  entropy_ = e > 0 ? e : 0.0;
  bit_ = final_dist_(1) > final_dist_(0) ? true : false;
}

void GraphNode::addEdge(std::shared_ptr<GraphEdge> e) {
  edges_.push_back(e);
}

}  // end namespace bp

}  // end namespace preimage
