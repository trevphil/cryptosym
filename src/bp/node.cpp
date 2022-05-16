/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include "bp/node.hpp"

#include <assert.h>
#include <math.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "bp/params.hpp"

namespace preimage {

namespace bp {

/***************************
 ******* GRAPH EDGE ********
 ***************************/

GraphEdge::GraphEdge(std::shared_ptr<GraphNode> n, std::shared_ptr<GraphFactor> f,
                     IODirection dir, bool neg)
    : node(n), factor(f), direction(dir), negated(neg) {}

std::string GraphEdge::toString() const {
  std::string dir_str = "";
  switch (direction) {
    case IODirection::None:
      dir_str = "None";
      break;
    case IODirection::Input:
      dir_str = "Input";
      break;
    case IODirection::Output:
      dir_str = "Output";
      break;
    case IODirection::Prior:
      dir_str = "Prior";
      break;
  }
  std::stringstream ss;
  if (negated) ss << "~ ";
  ss << node->toString() << " <-[" << dir_str << "]-> " << factor->toString();
  return ss.str();
}

/***************************
 ****** GRAPH FACTOR *******
 ***************************/

GraphFactor::GraphFactor(int i, BPFactorType t) : index_(i), t_(t) {
  switch (t_) {
    case BPFactorType::Prior:
      break;
    case BPFactorType::And:
      table_ = Eigen::MatrixXd::Zero(8, 4);
      // P(output = C | input1 = A, input2 = B)
      //        A  B  C  prob
      table_ << 0, 0, 0, BP_ONE, 0, 0, 1, BP_ZERO, 0, 1, 0, BP_ONE, 0, 1, 1, BP_ZERO, 1,
          0, 0, BP_ONE, 1, 0, 1, BP_ZERO, 1, 1, 0, BP_ZERO, 1, 1, 1, BP_ONE;
      break;
    case BPFactorType::Xor:
      table_ = Eigen::MatrixXd::Zero(8, 4);
      // P(output = C | input1 = A, input2 = B)
      //        A  B  C  prob
      table_ << 0, 0, 0, BP_ONE, 0, 0, 1, BP_ZERO, 0, 1, 0, BP_ZERO, 0, 1, 1, BP_ONE, 1,
          0, 0, BP_ZERO, 1, 0, 1, BP_ONE, 1, 1, 0, BP_ONE, 1, 1, 1, BP_ZERO;
      break;
    case BPFactorType::Or:
      table_ = Eigen::MatrixXd::Zero(8, 4);
      // P(output = C | input1 = A, input2 = B)
      //        A  B  C  prob
      table_ << 0, 0, 0, BP_ONE, 0, 0, 1, BP_ZERO, 0, 1, 0, BP_ZERO, 0, 1, 1, BP_ONE, 1,
          0, 0, BP_ZERO, 1, 0, 1, BP_ONE, 1, 1, 0, BP_ZERO, 1, 1, 1, BP_ONE;
      break;
    case BPFactorType::Maj:
      table_ = Eigen::MatrixXd::Zero(16, 5);
      // P(output = D | input1 = A, input2 = B, input3 = C)
      //        A  B  C  D  prob
      table_ << 0, 0, 0, 0, BP_ONE, 0, 0, 0, 1, BP_ZERO, 0, 0, 1, 0, BP_ONE, 0, 0, 1, 1,
          BP_ZERO, 0, 1, 0, 0, BP_ONE, 0, 1, 0, 1, BP_ZERO, 0, 1, 1, 0, BP_ZERO, 0, 1, 1,
          1, BP_ONE, 1, 0, 0, 0, BP_ONE, 1, 0, 0, 1, BP_ZERO, 1, 0, 1, 0, BP_ZERO, 1, 0,
          1, 1, BP_ONE, 1, 1, 0, 0, BP_ZERO, 1, 1, 0, 1, BP_ONE, 1, 1, 1, 0, BP_ZERO, 1,
          1, 1, 1, BP_ONE;
      break;
    case BPFactorType::Xor3:
      table_ = Eigen::MatrixXd::Zero(16, 5);
      // P(output = D | input1 = A, input2 = B, input3 = C)
      //        A  B  C  D  prob
      table_ << 0, 0, 0, 0, BP_ONE, 0, 0, 0, 1, BP_ZERO, 0, 0, 1, 0, BP_ZERO, 0, 0, 1, 1,
          BP_ONE, 0, 1, 0, 0, BP_ZERO, 0, 1, 0, 1, BP_ONE, 0, 1, 1, 0, BP_ONE, 0, 1, 1, 1,
          BP_ZERO, 1, 0, 0, 0, BP_ZERO, 1, 0, 0, 1, BP_ONE, 1, 0, 1, 0, BP_ONE, 1, 0, 1,
          1, BP_ZERO, 1, 1, 0, 0, BP_ONE, 1, 1, 0, 1, BP_ZERO, 1, 1, 1, 0, BP_ZERO, 1, 1,
          1, 1, BP_ONE;
      break;
  }
}

GraphFactor::~GraphFactor() {
  for (auto &e : edges_) e.reset();
}

std::string GraphFactor::ftype2str(BPFactorType t) {
  switch (t) {
    case BPFactorType::And:
      return "And";
    case BPFactorType::Prior:
      return "Prior";
    case BPFactorType::Xor:
      return "Xor";
    case BPFactorType::Or:
      return "Or";
    case BPFactorType::Maj:
      return "Maj";
    case BPFactorType::Xor3:
      return "Xor3";
  }

  throw std::invalid_argument("Unsupported factor type");
}

std::string GraphFactor::makeString(int index, BPFactorType t) {
  std::stringstream ss;
  ss << "Factor " << index << " " << ftype2str(t);
  return ss.str();
}

std::string GraphFactor::toString() const { return GraphFactor::makeString(index_, t_); }

int GraphFactor::index() const { return index_; }

BPFactorType GraphFactor::type() const { return t_; }

std::vector<std::shared_ptr<GraphEdge>> GraphFactor::edges() const { return edges_; }

void GraphFactor::initMessages() {
  assert(t_ != BPFactorType::Prior);
  const int l = edges_.size();
  int input_column = 0;
  for (int edge_idx = 0; edge_idx < l; edge_idx++) {
    std::shared_ptr<GraphEdge> edge = edges_.at(edge_idx);
    edge->m2n = Eigen::Vector2d::Ones() * 0.5;  // Init message

    switch (edge->direction) {
      case IODirection::None:
      case IODirection::Prior:
        throw std::exception();  // This should not happen
        break;
      case IODirection::Input:
        // The first columns of the probability table are for input variables
        // Important! We assume the input order doesn't matter (symmetric!)
        edge_index_for_table_column_[input_column] = edge_idx;
        input_column++;
        break;
      case IODirection::Output:
        // The second-last column (there are l + 1 columns) is the output
        edge_index_for_table_column_[l - 1] = edge_idx;
        break;
    }
  }
  // Each edge should map to a unique column
  assert(l == static_cast<int>(edge_index_for_table_column_.size()));
}

Eigen::MatrixXd GraphFactor::gatherIncoming() const {
  const int l = edges_.size();
  Eigen::MatrixXd msg_in = Eigen::MatrixXd::Zero(l, 2);
  for (int i = 0; i < l; ++i) {
    msg_in.row(i) = edges_.at(i)->m2f;
  }
  return msg_in;
}

void GraphFactor::factor2node() {
  assert(t_ != BPFactorType::Prior);
  const int l = edges_.size();
  const Eigen::MatrixXd msg_in = gatherIncoming();
  assert(l == msg_in.rows());
  assert(l == table_.cols() - 1);

  Eigen::MatrixXd tfill = table_.replicate(1, 1);
  const int n_rows = tfill.rows();

  for (int col = 0; col < l; col++) {
    const int edge_idx = edge_index_for_table_column_.at(col);
    const bool negated = edges_.at(edge_idx)->negated;
    Eigen::Array2d m = msg_in.row(edge_idx);
    for (int row = 0; row < n_rows; row++) {
      if ((!negated && table_(row, col) == 0) || (negated && table_(row, col) == 1)) {
        tfill(row, col) = m(0);
      } else {
        tfill(row, col) = m(1);
      }
    }
  }

  for (int col = 0; col < l; col++) {
    const int edge_idx = edge_index_for_table_column_.at(col);
    const bool negated = edges_.at(edge_idx)->negated;
    Eigen::MatrixXd tmp = tfill.replicate(1, 1);
    // Remove column "col" from tmp
    tmp.block(0, col, n_rows, l - col) = tmp.block(0, col + 1, n_rows, l - col);
    tmp.conservativeResize(n_rows, l);
    const Eigen::VectorXd p = tmp.rowwise().prod();
    double s0 = 0;
    double s1 = 0;
    for (int row = 0; row < table_.rows(); row++) {
      if ((!negated && table_(row, col) == 0) || (negated && table_(row, col) == 1)) {
        s0 += p(row);
      } else {
        s1 += p(row);
      }
    }
    edges_.at(edge_idx)->m2n(0) = s0;
    edges_.at(edge_idx)->m2n(1) = s1;
  }
}

void GraphFactor::addEdge(std::shared_ptr<GraphEdge> e) { edges_.push_back(e); }

/***************************
 ******* GRAPH NODE ********
 ***************************/

GraphNode::GraphNode(int i) : index_(i) {}

GraphNode::~GraphNode() {
  for (auto &e : edges_) e.reset();
}

void GraphNode::rescaleMatrix(Eigen::MatrixXd &m) {
  if (m.size() == 0) return;

  const Eigen::MatrixXd nonzero = (m.array() == 0.0).select(1.0, m);
  // const double dmin = std::numeric_limits<double>::epsilon();
  const double min_val = nonzero.minCoeff();
  // if (min_val < 100 * dmin) spdlog::warn("Very close to EPS!");

  const double max_val = m.maxCoeff();
  // const double dmax = std::numeric_limits<double>::max();
  // if (max_val > dmax / 100) spdlog::warn("Very close to DBL_MAX!");

  const double exp = (log10(max_val) - log10(min_val)) / (edges_.size() + 1);
  m *= pow(10, exp);
}

Eigen::Array2d GraphNode::stableColwiseProduct(const Eigen::MatrixXd &m) {
  Eigen::Array2d result = m.colwise().prod();
  return result;

  //   assert(m.cols() == 2);
  //   const int n_rows = m.rows();
  //   assert(n_rows >= 1);

  //   if (n_rows == 1) {
  //     Eigen::Array2d result;
  //     result(0) = m(0, 0);
  //     result(1) = m(0, 1);
  //     return result;
  //   }

  //   std::vector<double> col0, col1;
  //   col0.reserve(n_rows);
  //   col1.reserve(n_rows);

  //   for (int i = 0; i < n_rows; i++) {
  //     col0.push_back(m(i, 0)); col1.push_back(m(i, 1));
  //   }

  //   std::sort(col0.begin(), col0.end());
  //   std::sort(col1.begin(), col1.end());
  //   double prod0 = 1.0, prod1 = 1.0;

  //   if (col0.at(0) == 0 && col1.at(0) == 0) {
  //     // spdlog::error("Zero-sum for {}. Is there a contradiction?", toString());
  //     // assert(false);
  //     prod0 = 0.0;
  //     prod1 = 0.0;
  //   } else if (col0.at(0) == 0) {
  //     prod0 = 0.0;
  //     prod1 = 1.0;
  //   } else if (col1.at(0) == 0) {
  //     prod0 = 1.0;
  //     prod1 = 0.0;
  //   } else {
  //     for (int i = 0; i < n_rows; i++) {
  //       prod0 *= col0.at(i);
  //       prod1 *= col1.at(i);
  //       const double min_val = std::min(prod0, prod1);
  //       const double max_val = std::max(prod0, prod1);
  //       const double exp = (log10(max_val) - log10(min_val)) / 2;
  //       const double scalar = pow(10, exp);
  //       prod0 *= scalar;
  //       prod1 *= scalar;
  //     }
  //   }

  //   Eigen::Array2d result;
  //   result(0) = prod0;
  //   result(1) = prod1;
  //   return result;
}

std::string GraphNode::toString() const {
  std::stringstream ss;
  ss << "Node " << index_ << ": (" << final_dist_(0);
  ss << ", " << final_dist_(1) << ")";
  return ss.str();
}

int GraphNode::index() const { return index_; }

bool GraphNode::bit() const { return bit_; }

double GraphNode::entropy() const { return entropy_; }

double GraphNode::change() const { return change_; }

double GraphNode::distanceFromUndetermined() const {
  return std::min(std::fabs(final_dist_(0) - 0.5), std::fabs(final_dist_(1) - 0.5));
}

std::vector<std::shared_ptr<GraphEdge>> GraphNode::edges() const { return edges_; }

void GraphNode::initMessages() {
  directions_ = {};
  for (std::shared_ptr<GraphEdge> edge : edges_) {
    edge->m2f = Eigen::Vector2d::Ones() * 0.5;
    directions_.push_back(edge->direction);
  }

  in_factor_idx_ = {};
  out_factor_idx_ = {};
  all_factor_idx_ = {};
  for (int i = 0; i < static_cast<int>(directions_.size()); i++) {
    all_factor_idx_.push_back(i);
    switch (directions_.at(i)) {
      case IODirection::Input:
        in_factor_idx_.push_back(i);
        break;
      case IODirection::Output:
        out_factor_idx_.push_back(i);
        break;
      case IODirection::Prior:
        break;
      case IODirection::None:
        throw std::exception();  // This should not happen
        break;
    }
  }

  change_ = 0.0;
  entropy_ = 0.0;
  prev_dist_ = Eigen::Vector2d::Ones() * 0.5;
  final_dist_ = Eigen::Vector2d::Ones() * 0.5;

  const int l = edges_.size();
  prev_in_ = Eigen::MatrixXd::Zero(l, 2);
  prev_out_ = Eigen::MatrixXd::Zero(l, 2);
}

Eigen::MatrixXd GraphNode::gatherIncoming() const {
  const int l = edges_.size();
  Eigen::MatrixXd msg_in = Eigen::MatrixXd::Zero(l, 2);
  for (int i = 0; i < l; ++i) {
    // Clip to be >= 0 in case message behavior is funny
    msg_in(i, 0) = std::max(0.0, edges_.at(i)->m2n(0));
    msg_in(i, 1) = std::max(0.0, edges_.at(i)->m2n(1));
  }
  return msg_in;
}

void GraphNode::node2factor(IODirection target) {
  std::vector<int> targets;
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
      throw std::exception();  // This should not happen
      break;
  }

  const int l = edges_.size();
  if (targets.size() == 0 || l <= 1) return;
  Eigen::MatrixXd msg_in = gatherIncoming();

  const double d = BP_DAMPING;
  if (d < 1 && prev_in_.any()) {
    // Apply damping to the input
    msg_in = (msg_in * d) + (prev_in_ * (1 - d));
  }
  Eigen::MatrixXd msg_out = Eigen::MatrixXd::Zero(l, 2);

  for (int i : targets) {
    Eigen::MatrixXd tmp = msg_in.replicate(1, 1);
    // Remove row "i"
    tmp.block(i, 0, l - i - 1, 2) = tmp.block(i + 1, 0, l - i - 1, 2);
    tmp.conservativeResize(l - 1, 2);
    // rescaleMatrix(tmp);  // for numerical stability
    Eigen::Array2d p = stableColwiseProduct(tmp);  // tmp.colwise().prod();
    double s = p.sum();
    if (s == 0.0) {
      // spdlog::error("Zero-sum for {}. Is there a contradiction?", toString());
      // std::cout << tmp << std::endl;
      // assert(false);
      p = Eigen::Array2d::Ones();
      s = p.sum();
    }
    msg_out.row(i) = p / s;
  }

  for (int i : targets) {
    if (d < 1 && prev_out_.row(i).sum() > 0) {
      // Apply damping to the output
      msg_out.row(i) = (msg_out.row(i) * d) + (prev_out_.row(i) * (1 - d));
    }
    prev_out_.row(i) = msg_out.row(i);
    // Propagate to the edges
    edges_.at(i)->m2f = msg_out.row(i);
  }

  inlineNorm(msg_in);
  prev_in_ = msg_in;
}

void GraphNode::norm() {
  Eigen::MatrixXd Mm = gatherIncoming();
  // rescaleMatrix(Mm);  // for numerical stability
  Eigen::Array2d Zn = stableColwiseProduct(Mm);  // Mm.colwise().prod();
  double divisor = Zn.sum();
  if (divisor == 0.0) {
    // spdlog::error("Zero-sum for {}. Is there a contradiction?", toString());
    // std::cout << Mm << std::endl;
    // assert(false);
    Zn = Eigen::Array2d::Ones();
    divisor = Zn.sum();
  }
  const Eigen::Array2d P = Zn / divisor;
  final_dist_ = P;
  double e = 0.0;
  if (P(0) != 0) e += -P(0) * log2(P(0));
  if (P(1) != 0) e += -P(1) * log2(P(1));
  entropy_ = e > 0 ? e : 0.0;
  change_ = std::max(fabs(final_dist_(0) - prev_dist_(0)),
                     fabs(final_dist_(1) - prev_dist_(1)));
  prev_dist_ = final_dist_.replicate(1, 1);
  bit_ = final_dist_(1) > final_dist_(0) ? true : false;
}

void GraphNode::inlineNorm(const Eigen::MatrixXd &msg) {
  Eigen::Array2d Zn = msg.colwise().prod();
  double divisor = Zn.sum();
  if (divisor == 0.0) {
    Zn = Eigen::Array2d::Ones();
    divisor = Zn.sum();
  }
  const Eigen::Array2d P = Zn / divisor;
  final_dist_ = P;
  double e = 0.0;
  if (P(0) != 0) e += -P(0) * log2(P(0));
  if (P(1) != 0) e += -P(1) * log2(P(1));
  entropy_ = e > 0 ? e : 0.0;
  bit_ = final_dist_(1) > final_dist_(0) ? true : false;
}

void GraphNode::addEdge(std::shared_ptr<GraphEdge> e) { edges_.push_back(e); }

}  // end namespace bp

}  // end namespace preimage
