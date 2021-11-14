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

#include "cmsat/cmsat_solver.hpp"

#include <iostream>
#include <spdlog/spdlog.h>

namespace preimage {

CMSatSolver::CMSatSolver(bool verbose) : Solver(verbose) {}

CMSatSolver::~CMSatSolver() {
  if (solver_) delete solver_;
}

void CMSatSolver::addClause(const LogicGate &g) {
  const std::vector<std::vector<int>> clauses = g.cnf();
  for (const auto &clause : clauses) {
    std::vector<CMSat::Lit> cmsat_clause;
    for (int var : clause) cmsat_clause.push_back(getLit(var));
    solver_->add_clause(cmsat_clause);
  }
}

void CMSatSolver::addXorClause(const LogicGate &g) {
  int lsum = (g.output < 0 ? 1 : 0);
  std::vector<unsigned int> xor_clause = {
    static_cast<unsigned int>(std::abs(g.output) - 1)
  };

  for (int i = 0; i < static_cast<int>(g.inputs.size()); i++) {
    lsum += g.inputs[i] < 0 ? 1 : 0;
    xor_clause.push_back(static_cast<unsigned int>(std::abs(g.inputs[i]) - 1));
  }

  const bool xor_negated = (lsum % 2) == 1;
  solver_->add_xor_clause(xor_clause, xor_negated);
}

void CMSatSolver::initialize() {
  solver_ = new CMSat::SATSolver;
  solver_->set_num_threads(1);
  solver_->new_vars(num_vars_);

  if (verbose_)
    spdlog::info("Running cryptominisat5 (n={})", num_vars_);

  for (const LogicGate &g : gates_) {
    switch (g.t()) {
      case LogicGate::Type::and_gate:
      case LogicGate::Type::or_gate:
      case LogicGate::Type::maj_gate:
        addClause(g);
        break;
      case LogicGate::Type::xor_gate:
      case LogicGate::Type::xor3_gate:
        addXorClause(g);
        break;
    }
  }
}

std::unordered_map<int, bool> CMSatSolver::solveInternal() {
  std::vector<CMSat::Lit> assumptions;
  for (const auto &itr : observed_) {
    assert(itr.first > 0);
    CMSat::Lit lit(itr.first - 1, !itr.second);
    assumptions.push_back(lit);
  }

  CMSat::lbool ret = solver_->solve(&assumptions);
  assert(ret == CMSat::l_True);
  const auto final_model = solver_->get_model();

  std::unordered_map<int, bool> solution;
  for (int i = 1; i <= num_vars_; ++i) {
    solution[i] = (final_model[i - 1] == CMSat::l_True);
  }
  return solution;
}

}  // end namespace preimage
