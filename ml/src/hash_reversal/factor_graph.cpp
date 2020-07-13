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

#include "hash_reversal/factor_graph.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <deque>
#include <fstream>
#include <string>

namespace hash_reversal {

FactorGraph::FactorGraph(std::shared_ptr<Probability> prob, std::shared_ptr<Dataset> dataset,
                         std::shared_ptr<utils::Config> config)
    : prob_(prob), dataset_(dataset), config_(config) {
  spdlog::info("Initializing factor graph...");
  const auto start = utils::Convenience::time_since_epoch();

  setupUndirectedGraph();
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
      const bool both_inputs = dataset_->isHashInputBit(i) && dataset_->isHashInputBit(j);
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

void FactorGraph::setupFactors() {
  const size_t n = config_->num_rvs;
  rvs_ = std::vector<RandomVariable>(n);
  factors_ = std::vector<Factor>(n);

  for (size_t rv = 0; rv < n; ++rv) {
    factors_[rv].primary_rv = rv;
    for (size_t dependency : udg_[rv]) {
      factors_[rv].rv_dependencies.insert(dependency);
      rvs_[dependency].factor_indices.insert(rv);
    }
  }

  spdlog::info("\tFinished creating factors.");
}

FactorGraph::Prediction FactorGraph::predict(size_t rv_index) const {
  FactorGraph::Prediction prediction;
  double x = rv_initialization_(rv_index) + factor_messages_.col(rv_index).sum();
  prediction.log_likelihood_ratio = x;
  prediction.prob_bit_is_one = (x < 0) ? 1 : 0;
  return prediction;
}

std::vector<FactorGraph::Prediction> FactorGraph::marginals() const {
  const size_t n = config_->num_rvs;
  std::vector<Prediction> predictions;
  predictions.reserve(n);
  for (size_t rv = 0; rv < n; ++rv) predictions.push_back(predict(rv));
  return predictions;
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
    const auto &referenced_rvs = factor.referencedRVs();

    for (const auto &o : observed)
      if (referenced_rvs.count(o.rv_index) > 0) relevant.push_back(o);

    const double prob_rv_one = prob_->probOne(i, relevant, "auto-select");
    rv_initialization_(i) = std::log((1.0 - prob_rv_one) / prob_rv_one);
  }
}

void FactorGraph::runLBP(const std::vector<VariableAssignment> &observed) {
  setupLBP(observed);

  spdlog::info("\tStarting loopy BP...");
  const auto start = utils::Convenience::time_since_epoch();

  size_t itr = 0;
  for (itr = 0; itr < config_->lbp_max_iter; ++itr) {
    updateFactorMessages();
    updateRandomVariableMessages();
    const auto &marg = marginals();
    if (equal(previous_marginals_, marg)) break;
    previous_marginals_ = marg;
  }

  if (itr >= config_->lbp_max_iter) {
    spdlog::warn("\tLoopy BP did not converge, max iterations reached.");
  } else {
    spdlog::info("\tLoopy BP converged in {} iterations", itr + 1);
  }

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("\tLBP finished in {} seconds.", end - start);
}

bool FactorGraph::equal(const std::vector<FactorGraph::Prediction> &marginals1,
                        const std::vector<FactorGraph::Prediction> &marginals2, double tol) const {
  const size_t n = marginals1.size();
  if (n != marginals2.size()) return false;

  for (size_t i = 0; i < n; ++i) {
    if (std::abs(marginals1.at(i).log_likelihood_ratio - marginals2.at(i).log_likelihood_ratio) >
        tol) {
      return false;
    }
  }

  return true;
}

void FactorGraph::updateFactorMessages() {
  const Eigen::MatrixXd prev_factor_msg(factor_messages_);

  const auto rv_msg_tanh = (rv_messages_ / 2.0).array().tanh().matrix();
  for (size_t factor_index = 0; factor_index < factors_.size(); ++factor_index) {
    const auto &factor = factors_.at(factor_index);
    for (size_t rv_index : factor.rv_dependencies) {
      double prod = 1.0;
      for (size_t other_rv_index : factor.rv_dependencies) {
        if (rv_index != other_rv_index) prod *= rv_msg_tanh(other_rv_index, factor_index);
      }
      if (std::abs(prod) != 1.0) {
        factor_messages_(factor_index, rv_index) = 2.0 * std::atanh(prod);
      }
    }
  }

  const double damping = config_->lbp_damping;
  factor_messages_ = (factor_messages_ * damping) + (prev_factor_msg * (1 - damping));
}

void FactorGraph::updateRandomVariableMessages() {
  const Eigen::MatrixXd prev_rv_msg(rv_messages_);

  const auto col_sums = factor_messages_.colwise().sum();
  for (size_t rv_index = 0; rv_index < rvs_.size(); ++rv_index) {
    const auto &rv = rvs_.at(rv_index);
    for (size_t factor_index : rv.factor_indices) {
      double total = col_sums(rv_index) - factor_messages_(factor_index, rv_index);
      total += rv_initialization_(rv_index);
      rv_messages_(rv_index, factor_index) = total;
    }
  }

  const double damping = config_->lbp_damping;
  rv_messages_ = (rv_messages_ * damping) + (prev_rv_msg * (1 - damping));
}

void FactorGraph::printConnections() const {
  for (size_t i = 0; i < config_->num_rvs; ++i) {
    const auto &rv_neighbors = rvs_.at(i).factor_indices;
    const std::string rv_nb_str = utils::Convenience::set2str<size_t>(rv_neighbors);
    spdlog::info("\tRV {} is referenced by factors {}", i, rv_nb_str);
    const auto &fac_neighbors = factors_.at(i).rv_dependencies;
    const std::string fac_nb_str = utils::Convenience::set2str<size_t>(fac_neighbors);
    spdlog::info("\tFactor: RV {} depends on RVs {}", i, fac_nb_str);
  }
}

}  // end namespace hash_reversal