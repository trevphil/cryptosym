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

#include "cmsat_solver.hpp"

#include <spdlog/spdlog.h>

namespace dataset_generator {

CMSatSolver::CMSatSolver(const std::vector<Factor> &factors,
                         const std::vector<size_t> &input_indices)
    : Solver(factors, input_indices) {
  rv2idx_ = {};
  const unsigned int n = factors_.size();
  for (unsigned int i = 0; i < n; ++i) rv2idx_[factors_.at(i).output] = i;

  solver_ = new CMSat::SATSolver;
  solver_->set_num_threads(4);
  solver_->new_vars(n);
  spdlog::info("Initializing cryptominisat5 (n={})", n);

  std::vector<CMSat::Lit> clause;
  std::vector<unsigned int> xor_clause;

  for (const Factor &f : factors) {
    switch (f.t) {
      case Factor::Type::PriorFactor:
        break;
      case Factor::Type::SameFactor:
        clause = {// -out inp 0
                  CMSat::Lit(rv2idx_[f.output], true),
                  CMSat::Lit(rv2idx_[f.inputs.at(0)], false)};
        solver_->add_clause(clause);
        clause = {// out -inp 0
                  CMSat::Lit(rv2idx_[f.output], false),
                  CMSat::Lit(rv2idx_[f.inputs.at(0)], true)};
        solver_->add_clause(clause);
        break;
      case Factor::Type::NotFactor:
        clause = {// out inp 0
                  CMSat::Lit(rv2idx_[f.output], false),
                  CMSat::Lit(rv2idx_[f.inputs.at(0)], false)};
        solver_->add_clause(clause);
        clause = {// -out -inp 0
                  CMSat::Lit(rv2idx_[f.output], true),
                  CMSat::Lit(rv2idx_[f.inputs.at(0)], true)};
        solver_->add_clause(clause);
        break;
      case Factor::Type::AndFactor:
        clause = {// -out inp1 0
                  CMSat::Lit(rv2idx_[f.output], true),
                  CMSat::Lit(rv2idx_[f.inputs.at(0)], false)};
        solver_->add_clause(clause);
        clause = {// -out inp2 0
                  CMSat::Lit(rv2idx_[f.output], true),
                  CMSat::Lit(rv2idx_[f.inputs.at(1)], false)};
        solver_->add_clause(clause);
        clause = {// out -inp1 -inp2 0
                  CMSat::Lit(rv2idx_[f.output], false),
                  CMSat::Lit(rv2idx_[f.inputs.at(0)], true),
                  CMSat::Lit(rv2idx_[f.inputs.at(1)], true)};
        solver_->add_clause(clause);
        break;
      case Factor::Type::XorFactor:
        xor_clause = {rv2idx_[f.output], rv2idx_[f.inputs.at(0)],
                      rv2idx_[f.inputs.at(1)]};
        solver_->add_xor_clause(xor_clause, 0);
        break;
    }
  }
}

std::map<size_t, bool> CMSatSolver::solve(
    const std::map<size_t, bool> &observed) {
  std::vector<CMSat::Lit> assumptions;
  for (const auto &itr : observed) {
    const unsigned int idx = rv2idx_[itr.first];
    // The second argument is "is_negated". If bit=1, the bit is not negated.
    assumptions.push_back(CMSat::Lit(idx, !itr.second));
  }

  spdlog::info("Solving...");
  CMSat::lbool ret = solver_->solve(&assumptions);
  assert(ret == CMSat::l_True);
  const auto model = solver_->get_model();

  const size_t n = factors_.size();
  std::map<size_t, bool> solution;
  for (size_t i = 0; i < n; ++i) {
    solution[factors_.at(i).output] = (model[i] == CMSat::l_True);
  }
  return solution;
}

}  // end namespace dataset_generator
