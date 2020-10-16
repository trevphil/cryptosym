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

#include "hash_reversal/inference_tool.hpp"

#include <list>

#include <spdlog/spdlog.h>

namespace hash_reversal {

InferenceTool::InferenceTool(std::shared_ptr<Probability> prob,
                             std::shared_ptr<Dataset> dataset,
                             std::shared_ptr<utils::Config> config)
    : prob_(prob), dataset_(dataset), config_(config) {
  spdlog::info("Loading factors and random variables...");
  const auto start = utils::Convenience::time_since_epoch();

  const auto graph = dataset_->loadFactorGraph();
  rvs_ = graph.first;
  factors_ = graph.second;
  spdlog::info("\tCreated {} RVs and {} factors.", rvs_.size(), factors_.size());

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("Finished initializing factor graph in {} seconds.", end - start);

  if (config_->print_connections) printConnections();
}

InferenceTool::~InferenceTool() {}

VariableAssignments InferenceTool::propagateObserved(const VariableAssignments &observed) const {
  // TODO: Propagate backward --> forward --> backward ... until nothing new observed
  VariableAssignments fully_obs = observed;
  std::list<size_t> queue;
  std::set<size_t> seen;
  for (auto &itr : observed) queue.push_back(itr.first);

  while (queue.size() > 0) {
    const size_t rv = queue.front();
    queue.pop_front();
    seen.insert(rv);

    const bool rv_val = fully_obs.at(rv);
    const Factor &factor = factors_.at(rv);
    const std::string &f_type = factor.factor_type;
    const auto inputs = factor.inputRVs();

    if (f_type == "INV") {
      const size_t parent = inputs.at(0);
      fully_obs[parent] = !rv_val;
      if (seen.count(parent) == 0) queue.push_back(parent);
    } else if (f_type == "SAME") {
      const size_t parent = inputs.at(0);
      fully_obs[parent] = rv_val;
      if (seen.count(parent) == 0) queue.push_back(parent);
    } else if (f_type == "AND" && rv_val == true) {
      const size_t inp1 = inputs.at(0);
      const size_t inp2 = inputs.at(1);
      fully_obs[inp1] = true;
      fully_obs[inp2] = true;
      if (seen.count(inp1) == 0) queue.push_back(inp1);
      if (seen.count(inp2) == 0) queue.push_back(inp2);
    }
  }

  const size_t diff = fully_obs.size() - observed.size();
  spdlog::info("\tAble to solve {} additional RV values", diff);
  return fully_obs;
}

void InferenceTool::reconfigure(const VariableAssignments &observed) {
  (void)observed;
  spdlog::warn("reconfigure() function should be overridden in subclasses");
}

void InferenceTool::solve() {
  spdlog::warn("solve() function should be overridden in subclasses");
}

std::vector<InferenceTool::Prediction> InferenceTool::marginals() const {
  spdlog::warn("marginals() function should be overridden in subclasses");
  return {};
}

std::map<size_t, std::string> InferenceTool::factorTypes() const {
  std::map<size_t, std::string> f_types;
  for (auto &itr : factors_) {
    f_types[itr.first] = itr.second.factor_type;
  }
  return f_types;
}

void InferenceTool::printConnections() const {
  for (auto &itr : factors_) {
    const size_t rv = itr.first;
    const auto &rv_neighbors = rvs_.at(rv).factor_indices;
    const std::string rv_nb_str = utils::Convenience::set2str<size_t>(rv_neighbors);
    spdlog::info("\tRV {} is referenced by factors {}", rv, rv_nb_str);
    const auto &fac_neighbors = itr.second.referenced_rvs;
    const std::string fac_nb_str = utils::Convenience::set2str<size_t>(fac_neighbors);
    spdlog::info("\tFactor: RV {} depends on RVs {}", rv, fac_nb_str);
  }
}

}  // end namespace hash_reversal