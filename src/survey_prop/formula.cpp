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

#include "survey_prop/formula.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <random>

namespace preimage {

namespace sp {

// Fix var to value spin and possibly simplify the resulting formula
int fix(int var, int spin) {
  // Decrease the counter of un-assigned variables
  freespin--;

  // If the spin is already non-zero, it means the variable is already
  // fixed, so there is an issue!
  if (v[var].spin) return 1;

  v[var].spin = spin;

  if (ratio_bt_dec > 0) return 0;

  return simplify(var);
}

void addClause(int num_vars, int *vars, int *polarities) {
  // adds new clauses to the original formula
  ncl[num_vars] += 1;
  clause[M].type = num_vars;
  clause[M].lits = num_vars;

  struct literalstruct *newlitstruct = new literalstruct[num_vars];
  clause[M].literal = newlitstruct;
  for (int i = 0; i < num_vars; i++) {
    newlitstruct[i].var = vars[i];
    newlitstruct[i].bar = polarities[i];
    newlitstruct[i].eta = std::rand();
  }

  for (int i = 0; i < num_vars; i++) {
    int var = vars[i];
    v[var].clauses++;
    v[var].artificialclauses++;
    if (v[var].clauses > maxconn) {
      maxconn = v[var].clauses;
    }

    // Array of "clauselist" objects with # elements equal to the
    // number of clauses in which "var" appears. Each "clauselist"
    // object in the array contains a list of clauses.
    struct clauselist *newclauselist = new clauselist[v[var].clauses];

    // memcpy(dst, src, size)
    memcpy((newclauselist + 1), v[var].clauselist,
           sizeof(struct clauselist) * (v[var].clauses - 1));

    // add new clause at front
    newclauselist[0].clause = &clause[M];
    newclauselist[0].lit = i;
    v[var].clauselist = newclauselist;
  }

  M++;
}

int fixTwo(int var, int var2, int spin, int spin2) {
  // fixes the spin of two variables in the formula
  int clause[2] = {var, var2};
  int polarities[2] = {spin > 0 ? 0 : 1, spin2 > 0 ? 0 : 1};
  addClause(2, clause, polarities);
  return 0;
}

void printStats() {
  for (int i = 2; i <= maxlit; i++) {
    printf("\t%i %i-clauses\n", ncl[i], i);
  }
  printf("\t%i variables\n", freespin);
}

// Simplify after fixing variable "var"
int simplify(int var) {
  int l, aux = 0, i;
  struct clausestruct *c;
  int cl;

  // Iterate through each clause in which "var" is referenced
  for (cl = 0; cl < v[var].clauses; cl++) {
    c = v[var].clauselist[cl].clause;
    l = v[var].clauselist[cl].lit;
    if (c->type == 0) {
      continue;
    }
    ncl[c->type]--;
    // check if var renders SAT the clause
    if (c->literal[l].bar == (v[var].spin == -1)) {
      // Bar=1 indicates negated literal (lit=false),
      // as does when the spin is -1.
      // Bar=0 indicates normal literal (lit=true),
      // and (v[var].spin == -1) is 0 when lit != false.
      ncl[0]++;
      c->type = 0;
      continue;
    }
    ncl[(--(c->type))]++;
    // otherwise, check for further simplifications
    // type 0, contradiction?:
    if (c->type == 0) {
      printf("contradiction\n");
      exit(-1);
    }
    // no contradiction
    // type 1: unit clause propagation
    if (c->type == 1) {
      // find the unfixed literal
      for (i = 0; i < c->lits; i++) {
        if (v[aux = c->literal[i].var].spin == 0) break;
      }
      if (i == c->lits) continue;
      // a clause could be unit-clause-fixed by two different paths
      if (fix(aux, c->literal[i].bar ? -1 : 1)) return -1;
    }
  }
  return 0;
}

int solveSubsystem(std::map<int, bool> &solution) {
  // Solve sub-system for variables which have spin=0
  CMSat::SATSolver* solver = new CMSat::SATSolver;
  solver->new_vars(N);

  std::vector<CMSat::Lit> cmsat_clause;
  int cl, lit, var, is_negated;

  int n_added_clauses = 0;

  for (cl = 0; cl < M; cl++) {
    if (clause[cl].type == 0) continue;

    cmsat_clause.clear();
    for (lit = 0; lit < clause[cl].lits; lit++) {
      var = clause[cl].literal[lit].var;
      if (v[var].spin == 0) {
        is_negated = clause[cl].literal[lit].bar;
        cmsat_clause.push_back(CMSat::Lit(var - 1, is_negated));
      }
    }
    if (cmsat_clause.size() > 0) {
      solver->add_clause(cmsat_clause);
      n_added_clauses++;
    }
  }

  printf("CMSat: %d / %d clauses added\n", n_added_clauses, M);

  CMSat::lbool ret = solver->solve();
  if (ret != CMSat::l_True) return -1;  // Not satisfiable

  const auto model = solver->get_model();
  for (var = 1; var <= N; var++) {
    if (v[var].spin == 0) {
      solution[var] = (model[var - 1] == CMSat::l_True);
    }
  }

  printf("Solution for %d variables\n", (int)solution.size());

  return 0;
}

}  // end namespace sp

}  // end namespace preimage
