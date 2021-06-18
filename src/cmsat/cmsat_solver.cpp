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

#include <spdlog/spdlog.h>

namespace preimage {

CMSatSolver::CMSatSolver(bool verbose) : Solver(verbose) {}

CMSatSolver::~CMSatSolver() {
  if (solver_) delete solver_;
}

void CMSatSolver::initialize() {
  rv2idx_.clear();
  size_t i = 0;
  for (const size_t rv : input_indices_) rv2idx_[rv] = i++;
  for (const auto &itr : factors_) {
    const size_t rv = itr.first;
    const Factor &f = itr.second;
    if (f.valid) rv2idx_[rv] = i++;
  }

  const unsigned int n = rv2idx_.size();
  solver_ = new CMSat::SATSolver;
  solver_->set_num_threads(1);
  solver_->new_vars(n);
  if (verbose_) spdlog::info("Initializing cryptominisat5 (n={})", n);

  std::vector<CMSat::Lit> clause;
  std::vector<unsigned int> xor_clause;

  for (const auto &itr : factors_) {
    const Factor &f = itr.second;

    switch (f.t) {
      case Factor::Type::NotFactor:
        clause = {// out inp 0
                  CMSat::Lit(rv2idx_.at(f.output), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(0)), false)};
        solver_->add_clause(clause);
        clause = {// -out -inp 0
                  CMSat::Lit(rv2idx_.at(f.output), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(0)), true)};
        solver_->add_clause(clause);
        break;
      case Factor::Type::AndFactor:
        clause = {// -out inp1 0
                  CMSat::Lit(rv2idx_.at(f.output), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(0)), false)};
        solver_->add_clause(clause);
        clause = {// -out inp2 0
                  CMSat::Lit(rv2idx_.at(f.output), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(1)), false)};
        solver_->add_clause(clause);
        clause = {// out -inp1 -inp2 0
                  CMSat::Lit(rv2idx_.at(f.output), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(0)), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(1)), true)};
        solver_->add_clause(clause);
        break;
      case Factor::Type::XorFactor:
        xor_clause = {rv2idx_.at(f.output), rv2idx_.at(f.inputs.at(0)),
                      rv2idx_.at(f.inputs.at(1))};
        solver_->add_xor_clause(xor_clause, 0);
        break;
      case Factor::Type::OrFactor:
        clause = {// out -inp1 0
                  CMSat::Lit(rv2idx_.at(f.output), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(0)), true)};
        solver_->add_clause(clause);
        clause = {// out -inp2 0
                  CMSat::Lit(rv2idx_.at(f.output), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(1)), true)};
        solver_->add_clause(clause);
        clause = {// -out inp1 inp2 0
                  CMSat::Lit(rv2idx_.at(f.output), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(0)), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(1)), false)};
        solver_->add_clause(clause);
        break;
      case Factor::Type::MajFactor:
        clause = {// out -inp1 -inp2 0
                  CMSat::Lit(rv2idx_.at(f.output), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(0)), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(1)), true)};
        solver_->add_clause(clause);
        clause = {// out -inp1 -inp3 0
                  CMSat::Lit(rv2idx_.at(f.output), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(0)), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(2)), true)};
        solver_->add_clause(clause);
        clause = {// out -inp2 -inp3 0
                  CMSat::Lit(rv2idx_.at(f.output), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(1)), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(2)), true)};
        solver_->add_clause(clause);
        clause = {// -out inp1 inp2 0
                  CMSat::Lit(rv2idx_.at(f.output), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(0)), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(1)), false)};
        solver_->add_clause(clause);
        clause = {// -out inp1 inp3 0
                  CMSat::Lit(rv2idx_.at(f.output), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(0)), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(2)), false)};
        solver_->add_clause(clause);
        clause = {// -out inp2 inp3 0
                  CMSat::Lit(rv2idx_.at(f.output), true),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(1)), false),
                  CMSat::Lit(rv2idx_.at(f.inputs.at(2)), false)};
        solver_->add_clause(clause);
        break;
    }
  }
}

std::map<size_t, bool> CMSatSolver::solveInternal() {
  std::vector<CMSat::Lit> assumptions;
  for (const auto &itr : observed_) {
    const size_t rv = itr.first;
    if (rv2idx_.count(rv) == 0) continue;
    const unsigned int idx = rv2idx_.at(rv);
    // The second argument is "is_negated". If bit=1, the bit is not negated.
    assumptions.push_back(CMSat::Lit(idx, !itr.second));
  }

  CMSat::lbool ret = solver_->solve(&assumptions);
  assert(ret == CMSat::l_True);
  const auto final_model = solver_->get_model();

  std::map<size_t, bool> solution;
  for (const auto &itr : rv2idx_) {
      const size_t rv = itr.first;
      const unsigned int idx = itr.second;
      solution[rv] = (final_model[idx] == CMSat::l_True);
  }
  return solution;
}

}  // end namespace preimage
