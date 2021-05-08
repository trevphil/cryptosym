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

#include "ortools_cp/cp_solver.hpp"

namespace preimage {

ORToolsCPSolver::ORToolsCPSolver(bool verbose)
    : Solver(verbose) {}

void ORToolsCPSolver::setUsableLogicGates() const {
  config::use_xor = true;
  config::use_or = false;
}

std::string ORToolsCPSolver::rv2s(size_t rv) const {
  char buffer[64];
  sprintf(buffer, "%zu", rv);
  return buffer;
}

operations_research::sat::BoolVar ORToolsCPSolver::getVar(size_t rv) {
  if (rv2var_.count(rv) > 0) {
    return rv2var_[rv];
  } else {
    operations_research::sat::BoolVar var =
        cp_model_.NewBoolVar().WithName(rv2s(rv));
    rv2var_[rv] = var;
    return var;
  }
}

void ORToolsCPSolver::initialize() {
  cp_model_ = operations_research::sat::CpModelBuilder();
  rv2var_.clear();

  for (auto &itr : factors_) {
    const Factor &f = itr.second;
    if (!f.valid) continue;

    if (f.t == Factor::Type::NotFactor) {
      const auto inp = getVar(f.inputs.at(0));
      const auto out = getVar(f.output);
      cp_model_.AddNotEqual(inp, out);
    } else if (f.t == Factor::Type::SameFactor) {
      const auto inp = getVar(f.inputs.at(0));
      const auto out = getVar(f.output);
      cp_model_.AddEquality(inp, out);
    } else if (f.t == Factor::Type::AndFactor) {
      // Linearization of out = inp1 * inp2 for binary variables
      const auto inp1 = getVar(f.inputs.at(0));
      const auto inp2 = getVar(f.inputs.at(1));
      const auto out = getVar(f.output);
      cp_model_.AddLessOrEqual(out, inp1);
      cp_model_.AddLessOrEqual(out, inp2);
      operations_research::sat::LinearExpr expr =
          operations_research::sat::LinearExpr::BooleanSum({inp1, inp2});
      cp_model_.AddGreaterOrEqual(out, expr.AddConstant(-1));
    } else if (f.t == Factor::Type::XorFactor) {
      const auto inp1 = getVar(f.inputs.at(0));
      const auto inp2 = getVar(f.inputs.at(1));
      const auto out = getVar(f.output);
      const auto var = cp_model_.TrueVar();
      cp_model_.AddBoolXor({inp1, inp2, out, var});
    } else if (f.t != Factor::Type::PriorFactor) {
      spdlog::error("Factor '{}' not supported for solver '{}'",
                    f.toString(), solverName());
      assert(false);
    }
  }
}

std::map<size_t, bool> ORToolsCPSolver::solveInternal() {
  for (auto &itr : observed_) {
    const size_t rv = itr.first;
    const bool rv_val = itr.second;
    cp_model_.AddEquality(getVar(rv), rv_val);
  }

  operations_research::sat::Model model;
  operations_research::sat::SatParameters parameters;
  parameters.set_num_search_workers(4);
  model.Add(operations_research::sat::NewSatParameters(parameters));

  const auto response =
      operations_research::sat::SolveCpModel(cp_model_.Build(), &model);

  if (verbose_) {
    std::cout << operations_research::sat::CpSolverResponseStats(response)
              << std::endl;
  }

  const auto optimal = operations_research::sat::CpSolverStatus::OPTIMAL;
  const auto feasible = operations_research::sat::CpSolverStatus::FEASIBLE;

  std::map<size_t, bool> solution;
  if (response.status() == optimal || response.status() == feasible) {
    for (auto &it : rv2var_) {
      solution[it.first] =
          operations_research::sat::SolutionIntegerValue(response, it.second);
    }
  } else {
    spdlog::warn("Response status code is not OPTIMAL!");
  }
  return solution;
}

}  // end namespace preimage
