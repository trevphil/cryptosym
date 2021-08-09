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

#include <memory>
#include <assert.h>

#include "bp/bp_solver.hpp"
#include "bp/prior_factor.hpp"
#include "core/utils.hpp"

#include <spdlog/spdlog.h>

namespace preimage {

namespace bp {

BPSolver::BPSolver(bool verbose) : Solver(verbose) {}

void BPSolver::initialize() {
  g_ = Graph();
  g_.schedule_variable.clear();
  g_.schedule_variable.push_back({});
  g_.schedule_factor.clear();
  g_.schedule_factor.push_back({});
  int max_rv = 0;

  for (const LogicGate &gate : gates_) {
    const int rv = gate.output;
    assert(rv > 0);
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
      assert(inp != 0);
      std::shared_ptr<GraphNode> inp_node;
      if (!g_.hasNode(abs(inp))) {
        inp_node = std::shared_ptr<GraphNode>(new GraphNode(abs(inp)));
        g_.addNode(inp_node);
        max_rv = std::max(max_rv, abs(inp));
      } else {
        inp_node = g_.getNode(abs(inp));
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
    case LogicGate::Type::and_gate: return BPFactorType::And;
    case LogicGate::Type::xor_gate: return BPFactorType::Xor;
    case LogicGate::Type::or_gate: return BPFactorType::Or;
    case LogicGate::Type::maj_gate: return BPFactorType::Maj;
    case LogicGate::Type::xor3_gate: return BPFactorType::Xor3;
  }
}

std::unordered_map<int, bool> BPSolver::solveInternal() {
  std::vector<int> prior_rvs = {};
  for (const auto &itr : observed_) {
    const int rv = itr.first;
    assert(rv > 0);
    assert(g_.hasNode(rv));
    const bool bit_val = itr.second;
    std::shared_ptr<GraphFactor> fac(new GraphPriorFactor(rv, bit_val));
    g_.addFactor(fac);
    g_.connectFactorNode(fac, g_.getNode(rv), IODirection::Prior);
    prior_rvs.push_back(rv);
  }

  g_.initMessages();
  g_.spreadPriors(prior_rvs);

  while (g_.iterations() < BP_MAX_ITER) {
    const auto start = Utils::ms_since_epoch();
    g_.scheduledUpdate();
    g_.norm();
    g_.writeNodes();
    const auto end = Utils::ms_since_epoch();
    const double e = g_.entropySum();
    const double c = g_.maxChange();

    if (verbose_) {
      spdlog::info("Iter {}/{} - {} ms, entropy sum {:.3f}, max change {:.3f}",
                   g_.iterations(), BP_MAX_ITER, end - start, e, c);
    }

    if (e < BP_ENTROPY_THRESHOLD) {
      if (verbose_) {
        spdlog::info("Entropy thresh reached ({}), abort after iteration {}",
                     e, g_.iterations());
      }
      break;
    }

    if (c < BP_CHANGE_THRESHOLD) {
      if (verbose_) {
        spdlog::info("Change thresh reached ({}), converged after iteration {}",
                     c, g_.iterations());
      }
      break;
    }
  }

  if (verbose_) {
    spdlog::warn("Graph node number of resets: {}", GraphNode::num_resets);
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
