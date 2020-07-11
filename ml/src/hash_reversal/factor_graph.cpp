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

#include <cmath>
#include <string>
#include <deque>
#include <fstream>
#include <algorithm>

#include <spdlog/spdlog.h>

#include "hash_reversal/factor_graph.hpp"

namespace hash_reversal {

FactorGraph::FactorGraph(std::shared_ptr<Probability> prob,
                         std::shared_ptr<Dataset> dataset,
                         std::shared_ptr<utils::Config> config)
  : prob_(prob), dataset_(dataset), config_(config) {
  spdlog::info("Initializing factor graph...");
  const auto start = utils::Convenience::time_since_epoch();

  setupUndirectedGraph();
  setupDirectedGraph();
  setupFactors();

  const size_t n = config_->num_rvs;
  rv_messages_ = Eigen::MatrixXd::Zero(n, n);
  factor_messages_ = Eigen::MatrixXd::Zero(n, n);
  rv_initialization_ = Eigen::VectorXd::Zero(n);
  factor_initialization_ = Eigen::VectorXd::Zero(n);

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("Finished initializing factor graph in {} seconds.", end - start);

  if (config_->print_connections) printConnections();
}

void FactorGraph::setupUndirectedGraph() {
  std::ifstream data;
  data.open(config_->graph_file);
  std::string line;

  int i = 0;
  while (std::getline(data, line)) {
    std::stringstream line_stream(line);
    std::string cell;
    int j = 0;
    while (std::getline(line_stream, cell, ',')) {
      const bool nonzero = std::stod(cell) != 0;
      const bool both_inputs = dataset_->isHashInputBit(i) &&
                               dataset_->isHashInputBit(j);
      // Do not add self-loops or connections btwn. hash input bits
      if (nonzero && i != j && !both_inputs) {
        udg_[i].insert(j);
        udg_[j].insert(i);
      }
      ++j;
    }
    ++i;
  }

  data.close();
  spdlog::info("\tFinished creating undirected graph.");
}

void FactorGraph::setupDirectedGraph() {
  std::set<size_t> visited;
  std::set<size_t> unvisited;
  for (size_t i = 0; i < config_->num_rvs; ++i) unvisited.insert(i);

  std::deque<size_t> queue;
  for (size_t hash_input_bit : dataset_->hashInputBitIndices())
    queue.push_back(hash_input_bit);

  while (unvisited.size() > 0) {
    if (queue.empty()) {
      queue.push_back(*std::begin(unvisited));
    }

    while (!queue.empty()) {
      size_t rv = queue.front();
      queue.pop_front();
      if (visited.count(rv) > 0) continue;
      visited.insert(rv);
      unvisited.erase(rv);

      for (size_t neighbor : udg_.at(rv)) {
        queue.push_back(neighbor);
        const bool edge1_exists = dg_[rv].count(neighbor) > 0;
        const bool edge2_exists = dg_[neighbor].count(rv) > 0;
        if (!edge1_exists && !edge2_exists) {
          dg_[rv].insert(neighbor);
          reversed_dg_[neighbor].insert(rv);
        }
      }
    }
  }

  spdlog::info("\tFinished creating directed graph.");
}

void FactorGraph::setupFactors() {
  const size_t n = config_->num_rvs;
  rvs_ = std::vector<RandomVariable>(n);
  factors_ = std::vector<Factor>(n);

  for (size_t fac_idx = 0; fac_idx < n; ++fac_idx) {
    for (size_t dependency : reversed_dg_[fac_idx]) {
      factors_[fac_idx].rv_indices.insert(dependency);
      rvs_[dependency].factor_indices.insert(fac_idx);
    }
  }

  spdlog::info("\tFinished creating factors.");
}

FactorGraph::Prediction FactorGraph::predict(size_t rv_index) {
  FactorGraph::Prediction prediction;
  double x = rv_initialization_(rv_index) + factor_messages_.col(rv_index).sum();
  prediction.log_likelihood_ratio = x;
  prediction.prob_bit_is_one = (x < 0) ? 1 : 0;
  return prediction;
}

void FactorGraph::setupLBP(const std::vector<VariableAssignment> &observed) {
  spdlog::info("\tSetting up loopy BP...");
  const size_t n = config_->num_rvs;
  rv_messages_ = Eigen::MatrixXd::Zero(n, n);
  factor_messages_ = Eigen::MatrixXd::Zero(n, n);
  rv_initialization_ = Eigen::VectorXd::Zero(n);
  factor_initialization_ = Eigen::VectorXd::Zero(n);

  for (size_t i = 0; i < n; ++i) {
    std::vector<VariableAssignment> relevant;
    const auto &factor = factors_.at(i);

    std::set<size_t> neighbor_indices;
    neighbor_indices.insert(i);
    for (size_t neighbor_rv_index : factor.rv_indices)
      neighbor_indices.insert(neighbor_rv_index);

    for (const auto &o : observed)
      if (neighbor_indices.count(o.rv_index) > 0) relevant.push_back(o);

    const double prob_rv_one = prob_->probOne(i, relevant, "auto-select");
    rv_initialization_(i) = std::log((1.0 - prob_rv_one) / prob_rv_one);
  }
}

void FactorGraph::runLBP(const std::vector<VariableAssignment> &observed) {
  setupLBP(observed);

  spdlog::info("\tStarting loopy BP...");
  const auto start = utils::Convenience::time_since_epoch();

  // WTF? Factor messages control the bit predictions, so if they don't change,
  // the predictions shouldn't change.
  // Checking for sameness of factor messages = fast convergence, bad input prediction
  // Checking for sameness of RV messages = no convergence, but 100% accuracy on input
  //    But if you allow too many iterations in this case, again bad accuracy on input

  size_t itr = 0;
  auto prev_factor_messages = factor_messages_;
  while (itr < config_->lbp_max_iter) {
    updateFactorMessages();
    updateRandomVariableMessages();
    if (factor_messages_.isApprox(prev_factor_messages)) break;
    prev_factor_messages = factor_messages_;
    ++itr;
  }

  if (itr >= config_->lbp_max_iter) {
    spdlog::warn("\tLoopy BP did not converge, max iterations reached.");
  } else {
    spdlog::info("\tLoopy BP converged in {} iterations", itr + 1);
  }

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("\tLBP finished in {} seconds.", end - start);
}

void FactorGraph::updateFactorMessages() {
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
}

void FactorGraph::updateRandomVariableMessages() {
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

void FactorGraph::printConnections() const {
  for (size_t i = 0; i < config_->num_rvs; ++i) {
    const auto &fac_neighbors = factors_.at(i).rv_indices;
    const std::string fac_nb_str = utils::Convenience::set2str<size_t>(fac_neighbors);
    const auto &rv_neighbors = rvs_.at(i).factor_indices;
    const std::string rv_nb_str = utils::Convenience::set2str<size_t>(rv_neighbors);
    spdlog::info("\tRV {} is referenced by factors {}", i, rv_nb_str);
    spdlog::info("\tFactor: RV {} depends on RVs {}", i, fac_nb_str);
  }
}

}  // end namespace hash_reversal