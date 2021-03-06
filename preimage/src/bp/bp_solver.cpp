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

#include <spdlog/spdlog.h>

namespace preimage {

namespace bp {

BPSolver::BPSolver(const std::map<size_t, Factor> &factors,
                   const std::vector<size_t> &input_indices)
    : Solver(factors, input_indices) {
  g_ = Graph();
  size_t max_rv = 0;

  for (const auto &itr : factors_) {
    const size_t rv = itr.first;
    const Factor &f = itr.second;
    if (!f.valid) continue;
    FType t = convertFactorType(f.t);
    if (t == FType::None) continue;

    max_rv = std::max(max_rv, rv);
    std::shared_ptr<GraphFactor> fac(new GraphFactor(rv, t));
    g_.addFactor(fac);

    std::shared_ptr<GraphNode> out_node;
    if (!g_.hasNode(f.output)) {
      out_node = std::shared_ptr<GraphNode>(new GraphNode(f.output));
      g_.addNode(out_node);
    } else {
      out_node = g_.getNode(f.output);
    }
    g_.connectFactorNode(fac, out_node, IODirection::Output);

    for (size_t inp : f.inputs) {
      std::shared_ptr<GraphNode> inp_node;
      if (!g_.hasNode(inp)) {
        inp_node = std::shared_ptr<GraphNode>(new GraphNode(inp));
        g_.addNode(inp_node);
      } else {
        inp_node = g_.getNode(inp);
      }
      g_.connectFactorNode(fac, inp_node, IODirection::Input);
    }
  }

  g_.schedule_variable.clear();
  g_.schedule_variable.push_back({});
  g_.schedule_factor.clear();
  g_.schedule_factor.push_back({});

  for (size_t rv = 0; rv < max_rv; rv++) {
    if (g_.hasNode(rv)) g_.schedule_variable[0].push_back(g_.getNode(rv));
    if (g_.hasFactor(rv)) g_.schedule_factor[0].push_back(g_.getFactor(rv));
  }
}

FType BPSolver::convertFactorType(Factor::Type t) const {
  switch (t) {
  case Factor::Type::PriorFactor:
    // "Prior factors" (hash input bits) have no probabilistic interpretation
    return FType::None;
  case Factor::Type::AndFactor:
    return FType::And;
  case Factor::Type::NotFactor:
    return FType::Not;
  case Factor::Type::SameFactor:
    return FType::Same;
  case Factor::Type::XorFactor:
    return FType::Xor;
  }
}

std::map<size_t, bool> BPSolver::solveInternal() {
  g_.schedule_prior.clear();
  g_.schedule_prior.push_back({});
  for (const auto &itr : observed_) {
    const size_t rv = itr.first;
    const bool val = itr.second; // TODO: val is not used
    (void)val;
    assert(g_.hasNode(rv));
    std::shared_ptr<GraphNode> node = g_.getNode(rv);
    std::shared_ptr<GraphFactor> fac(new GraphFactor(rv, FType::Prior));
    g_.addFactor(fac);
    g_.connectFactorNode(fac, node, IODirection::Prior);
    g_.schedule_prior[0].push_back(fac);
  }

  g_.initMessages();
  g_.spreadPriors();

  while (g_.iterations() < BP_MAX_ITER) {
    spdlog::info("Iteration {}/{}", g_.iterations() + 1, BP_MAX_ITER);
    g_.scheduledUpdate();
    g_.norm();

    // TODO: set damping to 1.0 if entropy threshold in 1st layer is reached?

    const double e = g_.entropySum();
    if (e < BP_ENTROPY_THRESHOLD) {
      spdlog::info("Entropy thresh reached ({}), abort after iteration {}",
                   e, g_.iterations() + 1);
      break;
    }

    const double c = g_.maxChange();
    if (c < BP_CHANGE_THRESHOLD) {
      spdlog::info("Change thresh reached ({}), converged after iteration {}",
                   c, g_.iterations() + 1);
      break;
    }
  }

  std::map<size_t, bool> solution;
  for (const auto &itr : factors_) {
    const Factor &f = itr.second;
    if (!f.valid) continue;
    solution[f.output] = g_.getNode(f.output)->bit();
  }
  return solution;
}

}  // end namespace bp

}  // end namespace preimage
