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
                         std::shared_ptr<Dataset> dataset,
                         std::shared_ptr<utils::Config> config)
  : prob_(prob), dataset_(dataset), config_(config) {
  spdlog::info("Loading adjacency matrix from CSV...");
  const auto start = utils::Convenience::time_since_epoch();

  const size_t n = config->num_rvs;
  std::ifstream data;
  data.open(config->graph_file);
  std::string line;

  rvs_ = std::vector<RandomVariable>(n);
  factors_ = std::vector<Factor>(n);

  int fac_idx = 0;
  while (std::getline(data, line)) {
    std::stringstream line_stream(line);
    std::string cell;
    int rv_idx = 0;
    while (std::getline(line_stream, cell, ',')) {
      const bool nonzero = std::stod(cell) != 0;
      const bool make_connection = fac_idx == rv_idx ||
                                   (!dataset_->isHashInputBit(fac_idx) && nonzero);

      if (make_connection) {
        factors_[fac_idx].rv_indices.push_back(rv_idx);
        rvs_[rv_idx].factor_indices.push_back(fac_idx);
      }

      if (nonzero) boost::add_edge(fac_idx, rv_idx, udg_);
      ++rv_idx;
    }
    ++fac_idx;
  }

  rv_messages_ = Eigen::MatrixXd::Zero(n, n);
  factor_messages_ = Eigen::MatrixXd::Zero(n, n);
  rv_initialization_ = Eigen::VectorXd::Zero(n);
  factor_initialization_ = Eigen::VectorXd::Zero(n);

  data.close();

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("Finished loading adjacency matrix in {} seconds.", end - start);

  if (config_->print_connections) printConnections();
}

FactorGraph::Prediction FactorGraph::predict(size_t bit_index) {
  FactorGraph::Prediction prediction;
  double x = rv_initialization_(bit_index) + factor_messages_.col(bit_index).sum();
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
  graph_viz_counter_ = 0;

  for (size_t i = 0; i < n; ++i) {
    std::vector<VariableAssignment> relevant;
    const auto &factor = factors_.at(i);

    std::set<size_t> neighbor_indices;
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

  // Visualize the weight of all nodes in the undirected graph before LBP
  if (config_->graphviz) saveGraphViz();

  spdlog::info("\tStarting loopy BP...");
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

    // Visualize the weight of all nodes in the undirected graph after each iteration
    if (config_->graphviz) saveGraphViz();
  }

  if (itr >= config_->lbp_max_iter) {
    spdlog::warn("\tLoopy BP did not converge, max iterations reached.");
  } else {
    spdlog::info("\tLoopy BP converged in {} iterations", itr + 1);
  }

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("\tLBP finished in {} seconds.", end - start);
}

void FactorGraph::printConnections() const {
  for (size_t i = 0; i < config_->num_rvs; ++i) {
    auto fac_neighbors = factors_.at(i).rv_indices;
    std::sort(fac_neighbors.begin(), fac_neighbors.end());
    const std::string fac_nb_str = utils::Convenience::vec2str<size_t>(fac_neighbors);
    auto rv_neighbors = rvs_.at(i).factor_indices;
    std::sort(rv_neighbors.begin(), rv_neighbors.end());
    const std::string rv_nb_str = utils::Convenience::vec2str<size_t>(rv_neighbors);
    spdlog::info("\tRV {} is connected to factors {}", i, rv_nb_str);
    spdlog::info("\tFactor {} is connected to RVs {}", i, fac_nb_str);
  }
}

void FactorGraph::saveGraphViz() {
  for (size_t i = 0; i < config_->num_rvs; ++i) {
    udg_[i].weight = predict(i).log_likelihood_ratio;
  }

  auto w_map = boost::get(&VertexInfo::weight, udg_);
  boost::dynamic_properties dp;
  dp.property("weight", w_map);

  const std::string filename = config_->graphVizFile(graph_viz_counter_);
  std::ofstream viz_file;
  viz_file.open(filename);
  boost::write_graphml(viz_file, udg_, dp, true);
  viz_file.close();

  ++graph_viz_counter_;
}

}  // end namespace hash_reversal