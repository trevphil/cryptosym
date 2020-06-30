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

#include <set>
#include <cmath>
#include <string>
#include <fstream>
#include <algorithm>
#include <spdlog/spdlog.h>

#include "hash_reversal/factor_graph.hpp"

namespace hash_reversal {

FactorGraph::FactorGraph(std::shared_ptr<Probability> prob,
                         std::shared_ptr<utils::Config> config) : prob_(prob), config_(config) {
  spdlog::info("Loading adjacency matrix from CSV...");
  const auto start = utils::Convenience::time_since_epoch();

  const size_t n = config->num_rvs;
  std::ifstream data;
  data.open(config->graph_file);
  std::string line;

  rvs_ = std::vector<RandomVariable>(n);
  factors_ = std::vector<Factor>(n);

  int row = 0;
  while (std::getline(data, line)) {
    std::stringstream line_stream(line);
    std::string cell;
    int col = 0;
    while (std::getline(line_stream, cell, ',')) {
      if (std::stod(cell) != 0) {
        factors_[col].rv_indices.push_back(row);
        rvs_[row].factor_indices.push_back(col);
      }
      ++col;
    }
    ++row;
  }

  rv_messages_ = Eigen::MatrixXd::Zero(n, n);
  factor_messages_ = Eigen::MatrixXd::Zero(n, n);
  rv_initialization_ = Eigen::VectorXd::Zero(n);
  factor_initialization_ = Eigen::VectorXd::Zero(n);

  data.close();

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("Finished loading adjacency matrix in {} seconds.", end - start);
  // printConnections();
}

FactorGraph::Prediction FactorGraph::predict(size_t bit_index) {
  FactorGraph::Prediction prediction;
  double x = rv_initialization_(bit_index) + factor_messages_.col(bit_index).sum();
  prediction.log_likelihood_ratio = x;
  prediction.prob_bit_is_one = (x < 0) ? 1 : 0;
  return prediction;
}

void FactorGraph::setupLBP(const std::vector<VariableAssignment> &observed) {
  spdlog::info("Setting up loopy BP...");
  const size_t n = config_->num_rvs;
  rv_messages_ = Eigen::MatrixXd::Zero(n, n);
  factor_messages_ = Eigen::MatrixXd::Zero(n, n);
  rv_initialization_ = Eigen::VectorXd::Zero(n);
  factor_initialization_ = Eigen::VectorXd::Zero(n);

  for (size_t i = 0; i < n; ++i) {
    std::vector<VariableAssignment> relevant;
    const auto &factor = factors_.at(i);

    std::set<size_t> neighbor_indices;
    for (size_t neighbor_rv_index : factor.rv_indices)
      if (i != neighbor_rv_index) neighbor_indices.insert(neighbor_rv_index);

    for (const auto &o : observed)
      if (neighbor_indices.count(o.rv_index) > 0) relevant.push_back(o);

    relevant.push_back(VariableAssignment(i, 0));
    const size_t count0 = prob_->count(relevant);
    relevant.pop_back();
    relevant.push_back(VariableAssignment(i, 1));
    const size_t count1 = prob_->count(relevant);
    const size_t total = count0 + count1;

    double prob_rv_one = 0.5;
    if (total > 0) {
      const double eps = config_->epsilon;
      prob_rv_one = std::max(eps, std::min(1.0 - eps, double(count1) / total));
    }

    rv_initialization_(i) = std::log((1.0 - prob_rv_one) / prob_rv_one);
  }
}

void FactorGraph::runLBP(const std::vector<VariableAssignment> &observed) {
  setupLBP(observed);

  spdlog::info("Starting loopy BP...");
  const auto start = utils::Convenience::time_since_epoch();
  // TODO - Add convergence criteria

  size_t itr = 0;
  while (itr++ < config_->lbp_max_iter) {
    // Update factor messages
    const auto rv_msg_tanh = (rv_messages_ / 2.0).array().tanh().matrix();
    for (size_t factor_index = 0; factor_index < factors_.size(); ++factor_index) {
      const auto &factor = factors_.at(factor_index);
      for (size_t rv_index : factor.rv_indices) {
        double prod = 1.0;
        for (size_t other_rv_index : factor.rv_indices) {
          if (rv_index != other_rv_index) prod *= rv_msg_tanh(other_rv_index, factor_index);
        }
        if (std::abs(prod) != 1.0) {
          factor_messages_(factor_index, rv_index) = 2.0 * std::atanh(prod);
        }
      }
    }

    // Update random variable messages
    const auto col_sums = factor_messages_.colwise().sum();
    for (size_t rv_index = 0; rv_index < rvs_.size(); ++rv_index) {
      const auto &rv = rvs_.at(rv_index);
      for (size_t factor_index : rv.factor_indices) {
        double total = col_sums(rv_index) - factor_messages_(factor_index, rv_index);
        total += rv_initialization_(rv_index);
        rv_messages_(rv_index, factor_index) = total;
      }
    }
  }

  if (itr >= config_->lbp_max_iter) {
    spdlog::warn("Loopy BP did not converge, max iterations reached.");
  } else {
    spdlog::info("Loopy BP converged in {} iterations", itr + 1);
  }

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("\tLBP finished in {} seconds.", end - start);
}

void FactorGraph::printConnections() const {
  const size_t n = config_->num_rvs;
  for (size_t i = 0; i < n; ++i) {
    auto neighbors = factors_.at(i).rv_indices;
    std::sort(neighbors.begin(), neighbors.end());
    const std::string neighbors_str = utils::Convenience::vec2str<size_t>(neighbors);
    spdlog::info("\tRV {} is connected to {}", i, neighbors_str);
  }
}

}  // end namespace hash_reversal