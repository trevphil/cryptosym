/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include "cmsat/cmsat_solver.hpp"

#include "core/config.hpp"

namespace preimage {

CMSatSolver::CMSatSolver() : Solver(), solver_(nullptr) {}

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
      static_cast<unsigned int>(std::abs(g.output) - 1)};

  for (int i = 0; i < static_cast<int>(g.inputs.size()); i++) {
    lsum += g.inputs[i] < 0 ? 1 : 0;
    xor_clause.push_back(static_cast<unsigned int>(std::abs(g.inputs[i]) - 1));
  }

  const bool xor_negated = (lsum % 2) == 1;
  solver_->add_xor_clause(xor_clause, xor_negated);
}

void CMSatSolver::initializeSolver(int num_vars, const std::vector<LogicGate> &gates) {
  if (solver_) delete solver_;
  solver_ = new CMSat::SATSolver;
  solver_->set_num_threads(1);
  solver_->new_vars(num_vars);

  if (config::verbose) {
    printf("Running cryptominisat5 with %d variables\n", num_vars);
  }

  for (const LogicGate &g : gates) {
    switch (g.t()) {
      case LogicGate::Type::and_gate:
      case LogicGate::Type::or_gate:
      case LogicGate::Type::maj3_gate:
        addClause(g);
        break;
      case LogicGate::Type::xor_gate:
      case LogicGate::Type::xor3_gate:
        addXorClause(g);
        break;
    }
  }
}

std::unordered_map<int, bool> CMSatSolver::solve(
    const SymRepresentation &problem,
    const std::unordered_map<int, bool> &bit_assignments) {
  std::vector<CMSat::Lit> assumptions;
  for (const auto &itr : bit_assignments) {
    if (itr.first <= 0) {
      char err_msg[128];
      snprintf(err_msg, 128,
               "Bit assignments to solve() should use positive indices (got %d)",
               itr.first);
      throw std::invalid_argument(err_msg);
    }
    CMSat::Lit lit(itr.first - 1, !itr.second);
    assumptions.push_back(lit);
  }

  initializeSolver(problem.numVars(), problem.gates());
  CMSat::lbool ret = solver_->solve(&assumptions);
  if (ret != CMSat::l_True) {
    throw std::runtime_error("CryptoMiniSAT did not solve the problem!");
  }
  const auto final_model = solver_->get_model();

  std::unordered_map<int, bool> solution;
  for (int i = 1; i <= problem.numVars(); ++i) {
    solution[i] = (final_model[i - 1] == CMSat::l_True);
  }
  return solution;
}

}  // end namespace preimage
