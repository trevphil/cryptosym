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
    static_cast<unsigned int>(abs(g.output) - 1)
  };

  for (int i = 0; i < static_cast<int>(g.inputs.size()); i++) {
    lsum += g.inputs[i] < 0 ? 1 : 0;
    xor_clause.push_back(static_cast<unsigned int>(abs(g.inputs[i]) - 1));
  }

  const bool xor_negated = (lsum % 2) == 1;
  solver_->add_xor_clause(xor_clause, xor_negated);
}

void CMSatSolver::initialize() {
  solver_ = new CMSat::SATSolver;
  solver_->set_num_threads(1);
  solver_->new_vars(num_vars_);
  // solver_->set_no_simplify();

  /*
  void set_num_threads(unsigned n); // Number of threads to use. Must be set before any vars/clauses are added
  void set_allow_otf_gauss(); // Allow on-the-fly gaussian elimination

  void set_verbosity(unsigned verbosity = 0); // Default is 0, silent
  void set_verbosity_detach_warning(bool verb); // Default is 0, silent
  void set_default_polarity(bool polarity); // Default polarity when branching for all vars
  void set_no_simplify(); // Never simplify
  void set_no_simplify_at_startup(); // Doesn't simplify at start, faster startup time
  void set_no_equivalent_lit_replacement(); // Don't replace equivalent literals
  void set_no_bva(); // No bounded variable addition
  void set_no_bve(); // No bounded variable elimination
  void set_yes_comphandler(); // Allow component handler to work
  void set_greedy_undef(); // Try to set variables to l_Undef in solution
  void set_sampling_vars(std::vector<uint32_t>* sampl_vars);
  void set_timeout_all_calls(double secs); // max timeout on all subsequent solve() or simplify
  void set_up_for_scalmc(); // used to set the solver up for ScalMC configuration
  void set_single_run(); // we promise to call solve() EXACTLY once
  void set_intree_probe(int val);
  void set_sls(int val);
  void set_full_bve(int val);
  void set_full_bve_iter_ratio(double val);
  void set_scc(int val);
  void set_bva(int val);
  void set_distill(int val);
  void reset_vsids();
  void set_no_confl_needed(); // assumptions-based conflict will NOT be calculated for next solve run
  void set_xor_detach(bool val);
  */

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
