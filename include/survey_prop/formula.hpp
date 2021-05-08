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

#include <stdio.h>
#include <map>

#include <cryptominisat5/cryptominisat.h>

namespace preimage {

namespace sp {

// default parameters
#define ITERATIONS 1000  // max # iterations before giving up
#define EPSILONDEF 0.01  // convergence check
#define EPS (1.0e-16)

// fixing strategy (see function fix_*)
#define PARAMAGNET 0.01

struct pistruct {
  double p;  // product of (1 + eta) of literals of same sign
  double m;  // product of (1 + eta) of literals of opposite sign
  int pzero;
  int mzero;
};

struct weightstruct {
  double p;  // plus
  double z;  // zero
  double m;  // minus
};

struct literalstruct {
  int var;  // varnum=1,...,N
  unsigned char bar;  // bar=0,1
  double eta;  // eta of the literal Q(u) = delta(u) + eta*delta(u+bar ? -1 : 1)
};

struct clausestruct {
  struct literalstruct *literal;  // list of literals
  int type;  // type=0,1,... actual number of lits
  int lits;  // lits=0,1,... initial number of lits
};

struct clauselist {
  struct clausestruct *clause;  // in which clause
  int lit;  // in which literal on such clause
};

struct vstruct {
  int clauses;  // in how many clauses the var appears
  struct clauselist *clauselist; // list of clauses
  struct pistruct pi;  // product of (1+eta) of these clauses
  int spin;  // spin of the var, 0=unfixed
  int artificialclauses;  // in how many artificial implied clauses the var appears
};

void addClause(int num_vars, int *vars, int *polarities);

int simplify(int var);
int fix(int var, int spin);
int fixTwo(int var, int var2, int spin, int spin2);
int solveSubsystem(std::map<int, bool> &solution);
void printStats();
struct weightstruct normalize(struct weightstruct H);

// global system vars & structures
extern struct vstruct *v;            // all per var information
extern struct clausestruct *clause;  // all per clause information
extern double *magn;          // spin magnetization list (for fixing ordering)
extern int *perm;             // permutation, for update ordering
extern int M;                    // number of clauses
extern int N;                    // number of variables

// Mapping from variable index to # clauses referencing the variable
extern int *ncl;              // clause size histogram

extern int maxlit;               // maximum clause size
extern int freespin;                 // number of unfixed variables
extern double epsilon;  // convergence criterion

// Max # clauses that 1 variable is referenced in
extern int maxconn;

// auxiliary vars
extern int *list;
extern double *prod;

// flags & options
extern double percent;
extern int num_fix_per_step;
extern int verbose;
extern int iterations;
extern int K;
extern double rho;
extern double norho;

extern int max_iter;
extern int streamlining_iter;
extern int use_streamlining;
extern int disjunction_limit;
extern int maxclauses;
extern double ratio_bt_dec;
extern double *magn_sign;
extern double *pos_magn_sign;
extern double *neg_magn_sign;
extern double *free_magn_sign;

}  // end namespace sp

}  // end namespace preimage
