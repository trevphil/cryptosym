/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include "bp/bp_solver.hpp"

#include <assert.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include "bp/node.hpp"
#include "bp/params.hpp"
#include "bp/prior_factor.hpp"
#include "core/config.hpp"
#include "core/utils.hpp"

namespace preimage {

namespace bp {

BPSolver::BPSolver() : Solver() {}

void BPSolver::initializeGraph(const std::vector<LogicGate> &gates) {
  g_ = Graph();
  g_.schedule_variable.clear();
  g_.schedule_variable.push_back({});
  g_.schedule_factor.clear();
  g_.schedule_factor.push_back({});
  int max_rv = 0;

  for (const LogicGate &gate : gates) {
    const int rv = gate.output;
    const BPFactorType t = convertLogicGate(gate.t());

    std::shared_ptr<GraphFactor> fac(new GraphFactor(rv, t));
    g_.addFactor(fac);
    g_.schedule_factor[0].push_back(fac);

    std::shared_ptr<GraphNode> out_node;
    if (!g_.hasNode(rv)) {
      out_node = std::shared_ptr<GraphNode>(new GraphNode(rv));
      g_.addNode(out_node);
      max_rv = std::max(max_rv, rv);
    } else {
      out_node = g_.getNode(rv);
    }
    g_.connectFactorNode(fac, out_node, IODirection::Output);

    for (int inp : gate.inputs) {
      std::shared_ptr<GraphNode> inp_node;
      if (!g_.hasNode(std::abs(inp))) {
        inp_node = std::shared_ptr<GraphNode>(new GraphNode(std::abs(inp)));
        g_.addNode(inp_node);
        max_rv = std::max(max_rv, std::abs(inp));
      } else {
        inp_node = g_.getNode(std::abs(inp));
      }
      g_.connectFactorNode(fac, inp_node, IODirection::Input, inp < 0);
    }
  }

  for (int rv = 0; rv <= max_rv; ++rv) {
    if (g_.hasNode(rv)) {
      g_.schedule_variable[0].push_back(g_.getNode(rv));
    }
  }
}

BPFactorType BPSolver::convertLogicGate(LogicGate::Type t) const {
  switch (t) {
    case LogicGate::Type::and_gate:
      return BPFactorType::And;
    case LogicGate::Type::xor_gate:
      return BPFactorType::Xor;
    case LogicGate::Type::or_gate:
      return BPFactorType::Or;
    case LogicGate::Type::maj_gate:
      return BPFactorType::Maj;
    case LogicGate::Type::xor3_gate:
      return BPFactorType::Xor3;
  }

  char err_msg[256];
  snprintf(err_msg, 256, "Unsupported logic gate: %c", (char)t);
  throw std::invalid_argument(err_msg);
}

std::unordered_map<int, bool> BPSolver::solve(
    const SymRepresentation &problem,
    const std::unordered_map<int, bool> &bit_assignments) {
  initializeGraph(problem.gates());

  std::vector<int> prior_rvs = {};
  for (const auto &itr : bit_assignments) {
    const int rv = itr.first;
    if (rv <= 0) {
      char err_msg[128];
      snprintf(err_msg, 128,
               "Bit assignments to solve() should use positive indices (got %d)", rv);
      throw std::invalid_argument(err_msg);
    }
    if (!g_.hasNode(rv)) {
      char err_msg[128];
      snprintf(err_msg, 128, "Belief propagation graph is missing node for RV %d", rv);
      throw std::runtime_error(err_msg);
    }
    const bool bit_val = itr.second;
    std::shared_ptr<GraphFactor> fac(new GraphPriorFactor(rv, bit_val));
    g_.addFactor(fac);
    g_.connectFactorNode(fac, g_.getNode(rv), IODirection::Prior);
    prior_rvs.push_back(rv);
  }

  g_.initMessages();
  g_.spreadPriors(prior_rvs);

  while (g_.iterations() < BP_MAX_ITER) {
    const auto start = utils::ms_since_epoch();
    g_.scheduledUpdate();
    g_.norm();
    g_.writeNodes();
    const auto end = utils::ms_since_epoch();
    const double e = g_.entropySum();
    const double c = g_.maxChange();

    if (config::verbose) {
      const int runtime = static_cast<int>(end - start);
      printf("Iter %d/%d - %d ms, entropy sum %.3f, max change %.3f\n", g_.iterations(),
             BP_MAX_ITER, runtime, e, c);
    }

    if (e < BP_ENTROPY_THRESHOLD) {
      if (config::verbose) {
        printf("Entropy thresh reached (%.3f), abort after iteration %d\n", e,
               g_.iterations());
      }
      break;
    }

    if (c < BP_CHANGE_THRESHOLD) {
      if (config::verbose) {
        printf("Change thresh reached (%.3f), converged after iteration %d\n", c,
               g_.iterations());
      }
      break;
    }
  }

  std::unordered_map<int, bool> solution;
  for (auto &nodes : g_.schedule_variable) {
    for (std::shared_ptr<GraphNode> node : nodes) {
      solution[node->index()] = node->bit();
    }
  }
  return solution;
}

}  // end namespace bp

}  // end namespace preimage
