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

#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <memory>

#include <spdlog/spdlog.h>

#include "core/solver.hpp"
#include "survey_prop/formula.hpp"
#include "survey_prop/sp.hpp"

namespace preimage {

class SPSolver : public Solver {
 public:
  SPSolver(bool verbose) : Solver(verbose) {}

  std::string solverName() const override { return "Survey Propagation"; }

  void setUsableLogicGates() const override {
    config::use_xor = false;
    config::use_or = true;
  }

 protected:
  void initialize() override {
    // global system vars & structures
    sp::v = NULL;             // all per var information
    sp::clause = NULL;        // all per clause information
    sp::magn = NULL;          // spin magnetization list (for fixing ordering)
    sp::perm = NULL;          // permutation, for update ordering
    sp::M = 0;                // number of clauses
    sp::N = 0;                // number of variables

    // Mapping from variable index to # clauses referencing the variable
    sp::ncl = NULL;            // clause size histogram

    sp::maxlit = 0;            // maximum clause size
    sp::freespin = 0;          // number of unfixed variables
    sp::epsilon = EPSILONDEF;  // convergence criterion

    // Max # clauses that 1 variable is referenced in
    sp::maxconn = 0;

    // auxiliary vars
    sp::list = NULL;
    sp::prod = NULL;

    // flags & options
    sp::percent = 0.0;
    sp::num_fix_per_step = 1;
    sp::verbose = 0;
    sp::iterations = ITERATIONS;
    sp::rho = 0;
    sp::norho = 1;

    sp::max_iter = 250;
    sp::streamlining_iter = 90;
    sp::use_streamlining = (sp::streamlining_iter > 0);
    sp::disjunction_limit = 2;
    sp::maxclauses = 100000;
    sp::ratio_bt_dec = 0.0;
    sp::magn_sign = NULL;
    sp::pos_magn_sign = NULL;
    sp::neg_magn_sign = NULL;
    sp::free_magn_sign = NULL;

    std::set<size_t> rv_set;
    sp::M += observed_.size();
    for (auto &itr : factors_) {
      const Factor &f = itr.second;
      if (!f.valid || f.t == Factor::Type::PriorFactor) continue;

      rv_set.insert(f.output);
      for (size_t inp : f.inputs) rv_set.insert(inp);

      switch (f.t) {
      case Factor::Type::PriorFactor:
        spdlog::error("Prior factors should be filtered out");
        assert(false);
        break;
      case Factor::Type::SameFactor:
      case Factor::Type::NotFactor:
        assert(f.inputs.size() == 1);
        sp::M += 2;
        break;
      case Factor::Type::AndFactor:
      case Factor::Type::OrFactor:
        assert(f.inputs.size() == 2);
        sp::M += 3;
        break;
      case Factor::Type::XorFactor:
        spdlog::error("XOR not yet implemented for survey propagation");
        assert(false);
        break;
      }
    }

    rv2var_.clear();
    int var = 1;
    for (size_t rv : rv_set) rv2var_[rv] = var++;
    rv_set.clear();

    sp::N = static_cast<int>(rv2var_.size());
    sp::v = new sp::vstruct[sp::N + 1];
    sp::clause = new sp::clausestruct[sp::M + 100 * sp::N];
    sp::freespin = sp::N;

    spdlog::info("N={}, M={}", sp::N, sp::M);
    spdlog::info("M/N ratio: {}", ((double)sp::M) / sp::N);

    for (var = 1; var <= sp::N; var++) {
      sp::v[var].clauses = 0;
      sp::v[var].artificialclauses = 0;
      sp::v[var].spin = 0;
      sp::v[var].pi.p = 0;
      sp::v[var].pi.m = 0;
      sp::v[var].pi.pzero = 0;
      sp::v[var].pi.mzero = 0;
    }

    var = 0;
    int cl = 0, literals = 0;

    for (auto &itr : observed_) {
      if (rv2var_.count(itr.first) > 0)
        prepareClause({rv2var_.at(itr.first)}, literals, cl);
    }

    for (auto &itr : factors_) {
      const Factor &f = itr.second;
      if (!f.valid || f.t == Factor::Type::PriorFactor) continue;

      int out, inp1, inp2;

      switch (f.t) {
      case Factor::Type::PriorFactor:
        spdlog::error("Should not have prior factor");
        assert(false);
        break;
      case Factor::Type::SameFactor:
      case Factor::Type::NotFactor:
        out = rv2var_.at(f.output);
        inp1 = rv2var_.at(f.inputs.at(0));
        prepareClause({out, inp1}, literals, cl);
        prepareClause({out, inp1}, literals, cl);
        break;
      case Factor::Type::AndFactor:
      case Factor::Type::OrFactor:
        out = rv2var_.at(f.output);
        inp1 = rv2var_.at(f.inputs.at(0));
        inp2 = rv2var_.at(f.inputs.at(1));
        prepareClause({out, inp1}, literals, cl);
        prepareClause({out, inp2}, literals, cl);
        prepareClause({out, inp1, inp2}, literals, cl);
        break;
      case Factor::Type::XorFactor:
        spdlog::error("XOR is not yet implemented for survey propagation");
        assert(false);
        break;
      }
    }

    sp::ncl = new int[sp::maxlit + 1];
    struct sp::literalstruct *allliterals = new sp::literalstruct[literals];
    struct sp::clauselist *allclauses = new sp::clauselist[literals];
    if (!allliterals || !allclauses) {
      spdlog::error("Not enough memory to allocate CNF formula!");
      return;
    }

    for (var = 1; var <= sp::N; var++) {
      if (sp::v[var].clauses > 0) {
        sp::v[var].clauselist = allclauses;
        allclauses += sp::v[var].clauses;
        sp::v[var].clauses = 0;
      } else {
        spdlog::error("var {} appears in 0 clauses!", var);
        assert(false);
      }
    }

    for (cl = 0; cl < sp::M; cl++) {
      sp::clause[cl].literal = allliterals;
      allliterals += sp::clause[cl].lits;
    }

    cl = 0;
    for (auto &itr : observed_) {
      if (rv2var_.count(itr.first) > 0)
        addClause({rv2var_.at(itr.first)}, {!itr.second}, cl);
    }
    for (auto &itr : factors_) {
      const Factor &f = itr.second;
      if (!f.valid || f.t == Factor::Type::PriorFactor) continue;

      switch (f.t) {
      case Factor::Type::PriorFactor:
        break;
      case Factor::Type::SameFactor:
        assert(f.inputs.size() == 1);
        addClause({rv2var_.at(f.output), rv2var_.at(f.inputs.at(0))},
                  {true, false}, cl);
        addClause({rv2var_.at(f.output), rv2var_.at(f.inputs.at(0))},
                  {false, true}, cl);
        break;
      case Factor::Type::NotFactor:
        assert(f.inputs.size() == 1);
        addClause({rv2var_.at(f.output), rv2var_.at(f.inputs.at(0))},
                  {false, false}, cl);
        addClause({rv2var_.at(f.output), rv2var_.at(f.inputs.at(0))},
                  {true, true}, cl);
        break;
      case Factor::Type::AndFactor:
        assert(f.inputs.size() == 2);
        addClause({rv2var_.at(f.output), rv2var_.at(f.inputs.at(0))},
                  {true, false}, cl);
        addClause({rv2var_.at(f.output), rv2var_.at(f.inputs.at(1))},
                  {true, false}, cl);
        addClause({rv2var_.at(f.output), rv2var_.at(f.inputs.at(0)), rv2var_.at(f.inputs.at(1))},
                  {false, true, true}, cl);
        break;
      case Factor::Type::OrFactor:
        assert(f.inputs.size() == 2);
        addClause({rv2var_.at(f.output), rv2var_.at(f.inputs.at(0))},
                  {false, true}, cl);
        addClause({rv2var_.at(f.output), rv2var_.at(f.inputs.at(1))},
                  {false, true}, cl);
        addClause({rv2var_.at(f.output), rv2var_.at(f.inputs.at(0)), rv2var_.at(f.inputs.at(1))},
                  {true, false, false}, cl);
        break;
      case Factor::Type::XorFactor:
        spdlog::error("XOR not yet implemented for survey propagation");
        assert(false);
        break;
      }
    }
  }

  void prepareClause(const std::vector<int> &vars,
                     int &literals, int &cl) {
    const int n = static_cast<int>(vars.size());

    for (int var : vars) {
      // Count number of clauses in which each variable appears
      sp::v[var].clauses++;

      // Count max # clauses in which a variable appears
      if (sp::v[var].clauses > sp::maxclauses) {
        sp::maxclauses = sp::v[var].clauses;
      }
    }

    literals += n;  // <-- total # literals in all clauses combined
    sp::clause[cl].type = n;
    sp::clause[cl].lits = n;
    cl++;

    // Track max # literals in any single clause
    if (sp::maxlit < n) sp::maxlit = n;
  }

  void addClause(const std::vector<int> &vars,
                 const std::vector<bool> &negated, int &cl) {
    const int n = static_cast<int>(vars.size());
    assert(n == static_cast<int>(negated.size()));

    for (int lit = 0; lit < n; lit++) {
      const int var = vars.at(lit);
      sp::v[var].clauselist[sp::v[var].clauses].clause = sp::clause + cl;
      sp::v[var].clauselist[sp::v[var].clauses].lit = lit;
      sp::v[var].clauses++;
      sp::clause[cl].literal[lit].var = var;
      sp::clause[cl].literal[lit].bar = negated.at(lit);
    }

    sp::ncl[n]++; // Track how many clauses have "n" literals in them
    cl++;
  }

  std::map<size_t, bool> solveInternal() override {
    // Run survey propagation, fill solution with var --> bool mapping
    std::map<int, bool> literal_assignments;
    sp::run(literal_assignments);

    // Convert indices from CNF indices (var) to RV indices
    std::map<size_t, bool> solution;
    for (auto &itr : rv2var_) {
      const size_t rv = itr.first;
      const int var = itr.second;
      solution[rv] = literal_assignments.at(var);
    }
    return solution;
  }

 private:
  std::unordered_map<size_t, int> rv2var_;
};

}  // end namespace preimage
