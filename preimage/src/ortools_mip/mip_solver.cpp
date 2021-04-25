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

#include "ortools_mip/mip_solver.hpp"

namespace preimage {

ORToolsMIPSolver::ORToolsMIPSolver(bool verbose)
    : Solver(verbose) {}

void ORToolsMIPSolver::setUsableLogicGates() const {
  config::use_xor = false;
  config::use_or = false;
}

std::string ORToolsMIPSolver::rv2s(size_t rv) const {
  char buffer[64];
  sprintf(buffer, "%zu", rv);
  return buffer;
}

operations_research::MPVariable* ORToolsMIPSolver::getVar(size_t rv) {
  if (rv2var_.count(rv) > 0) {
    return rv2var_[rv];
  } else {
    rv2var_[rv] = solver_->MakeBoolVar(rv2s(rv));
    return rv2var_[rv];
  }
}

void ORToolsMIPSolver::initialize() {
  solver_ = std::unique_ptr<operations_research::MPSolver>(
      operations_research::MPSolver::CreateSolver("CBC"));
  rv2var_.clear();

  const double infinity = solver_->infinity();

  for (auto &itr : factors_) {
    const Factor &f = itr.second;
    if (!f.valid) continue;

    if (f.t == Factor::Type::NotFactor) {
      const auto inp = getVar(f.inputs.at(0));
      const auto out = getVar(f.output);
      // 1 - inp = out  -->  inp + out = 1
      auto c = solver_->MakeRowConstraint(1, 1);
      c->SetCoefficient(inp, 1);
      c->SetCoefficient(out, 1);
    } else if (f.t == Factor::Type::SameFactor) {
      const auto inp = getVar(f.inputs.at(0));
      const auto out = getVar(f.output);
      // inp = out  -->  inp - out = 0
      auto c = solver_->MakeRowConstraint(0, 0);
      c->SetCoefficient(inp, 1);
      c->SetCoefficient(out, -1);
    } else if (f.t == Factor::Type::AndFactor) {
      // Linearization of out = inp1 * inp2 for binary variables
      // out <= inp1  -->  out - inp1 <= 0
      // out <= inp2  -->  out - inp2 <= 0
      // out >= inp1 + inp2 - 1  -->  out - inp1 - inp2 >= -1
      //                         -->  inp1 + inp2 - out <= 1
      const auto inp1 = getVar(f.inputs.at(0));
      const auto inp2 = getVar(f.inputs.at(1));
      const auto out = getVar(f.output);
      auto c0 = solver_->MakeRowConstraint(-infinity, 0);
      c0->SetCoefficient(out, 1);
      c0->SetCoefficient(inp1, -1);
      auto c1 = solver_->MakeRowConstraint(-infinity, 0);
      c1->SetCoefficient(out, 1);
      c1->SetCoefficient(inp2, -1);
      auto c2 = solver_->MakeRowConstraint(-infinity, 1);
      c2->SetCoefficient(inp1, 1);
      c2->SetCoefficient(inp2, 1);
      c2->SetCoefficient(out, -1);
    } else if (f.t != Factor::Type::PriorFactor) {
      spdlog::error("Factor '{}' not supported for solver '{}'",
                    f.toString(), solverName());
      assert(false);
    }
  }
}

std::map<size_t, bool> ORToolsMIPSolver::solveInternal() {
  for (auto &itr : observed_) {
    const size_t rv = itr.first;
    const bool rv_val = itr.second;
    auto c = solver_->MakeRowConstraint(rv_val, rv_val);
    c->SetCoefficient(getVar(rv), 1);
  }

  // (void)(solver_->SetNumThreads(4));

  auto objective = solver_->MutableObjective();
  for (size_t rv : input_indices_) {
    objective->SetCoefficient(getVar(rv), 1);
  }
  objective->SetMaximization();

  if (verbose_) {
    spdlog::info("Number of variables: {}", solver_->NumVariables());
    spdlog::info("Number of constraints: {}", solver_->NumConstraints());
  }

  const auto response = solver_->Solve();

  std::map<size_t, bool> solution;
  if (response != operations_research::MPSolver::OPTIMAL) {
    if (response == 2) {
      spdlog::error("Problem is infeasible!");
    } else {
      spdlog::warn("Response status code ({}) is not OPTIMAL!", response);
    }
    return solution;
  }

  for (auto &it : rv2var_) {
    solution[it.first] = it.second->solution_value();
  }

  if (verbose_) {
    spdlog::info("Problem solved in {} ms", solver_->wall_time());
    spdlog::info("Problem solved in {} iterations", solver_->iterations());
    spdlog::info("Problem solved in {} branch-and-bound nodes", solver_->nodes());
  }

  return solution;
}

}  // end namespace preimage
