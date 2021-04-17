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

#pragma once

#include "ortools/sat/cp_model.h"
#include "ortools/sat/model.h"
#include "ortools/sat/sat_parameters.pb.h"

#include <map>
#include <vector>
#include <assert.h>

#include "core/solver.hpp"

using namespace operations_research;
using namespace sat;

namespace preimage {

class ORToolsCPSolver : public Solver {
 public:
  ORToolsCPSolver(bool verbose) : Solver(verbose) {}

  std::string solverName() const override { return "ortools CP"; }

 protected:
  inline std::string rv2s(size_t rv) const {
    char buffer[64];
    sprintf(buffer, "%zu", rv);
    return buffer;
  }

  inline BoolVar getVar(size_t rv) {
    if (rv2var_.count(rv) > 0) {
      return rv2var_[rv];
    } else {
      BoolVar var = cp_model_.NewBoolVar().WithName(rv2s(rv));
      rv2var_[rv] = var;
      return var;
    }
  }

  void initialize() override {
    cp_model_ = CpModelBuilder();
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
        LinearExpr expr = LinearExpr::BooleanSum({inp1, inp2});
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

  std::map<size_t, bool> solveInternal() override {
    for (auto &itr : observed_) {
      const size_t rv = itr.first;
      const bool rv_val = itr.second;
      cp_model_.AddEquality(getVar(rv), rv_val);
    }

    Model model;

    SatParameters parameters;
    // parameters.set_max_time_in_seconds(10.0);
    parameters.set_num_search_workers(4);
    model.Add(NewSatParameters(parameters));

    const CpSolverResponse response = SolveCpModel(cp_model_.Build(), &model);
    if (verbose_) {
      std::cout << CpSolverResponseStats(response) << std::endl;
    }

    std::map<size_t, bool> solution;
    if (response.status() == CpSolverStatus::OPTIMAL) {
      for (auto &itr : rv2var_) {
        solution[itr.first] =
            SolutionIntegerValue(response, itr.second);
      }
    }

    return solution;
  }

 private:
  CpModelBuilder cp_model_;
  std::map<size_t, BoolVar> rv2var_;
};

}  // end namespace preimage
