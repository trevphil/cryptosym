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

#include "survey_prop/sp.hpp"

#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <random>

#include "survey_prop/formula.hpp"

namespace preimage {

namespace sp {

//global system vars & structures
struct vstruct *v=NULL; //all per var information
struct clausestruct *clause=NULL; //all per clause information
double *magn=NULL; //spin magnetization list (for fixing ordering)
int *perm=NULL; //permutation, for update ordering
int M=0; //number of clauses
int N=0; //number of variables
int *ncl=NULL; //clause size histogram
int maxlit=0; //maximum clause size
int freespin; //number of unfixed variables
double epsilon=EPSILONDEF; //convergence criterion
int maxconn=0; //maximum connectivity

//auxiliary vars
int *list=NULL;
double *prod=NULL;

//flags & options
double percent=0;
int num_fix_per_step=1;
int verbose=0;
int iterations=ITERATIONS;
double rho=0;
double norho=1;

int max_iter=250;
int streamlining_iter=90;
int use_streamlining=0;
int disjunction_limit=2;
int maxclauses=100000;
double ratio_bt_dec=0.0;
double *magn_sign=NULL;
double *pos_magn_sign=NULL;
double *neg_magn_sign=NULL;
double *free_magn_sign=NULL;

int run(std::map<int, bool> &solution) {
  if (use_streamlining) {
    printf("running survey inspired streamlining\n");
  } else {
    printf("running survey inspired decimation\n");
  }

  // define num_fix_per_step if based on percent
  if (percent) {
    num_fix_per_step = N * percent / 100.0;
  }

  maxclauses = M + 100 * N;

  initMemory();
  eliminateStandaloneClauses();  // Simplify formula
  randomizeEta();

  for (int iter_num = 0; iter_num < max_iter; iter_num++) {
    if (!runSurveyPropagation()) {
      printf("%s\n", "survey propagation failed!");
      exit(-1);
    }

    int oldfreespin = freespin;
    int num_in_list;
    double average_magnetization;

    buildList(&index_biased, &num_in_list, &average_magnetization);

    if (reachedSolverThreshold(average_magnetization) ||
        iter_num == max_iter - 1) {
      // Solve and exit!
      for (int var = 1; var <= N; var++) {
        if (v[var].spin != 0) {
          // We already have values for variables with non-zero spin
          solution[var] = v[var].spin == -1 ? false : true;
        }
      }

      // Solve sub-system with variables which have spin=0
      if (solveSubsystem(solution) != 0) {
        printf("%s\n", "CryptoMiniSAT did not find a solution!");
        return -1;
      }

      return 0;
    }

    if (use_streamlining && iter_num > streamlining_iter) {
      use_streamlining = 0;
    }

    fixChunk(num_in_list, num_fix_per_step);

    if (verbose) {
      printf("fixed %i biased var%s (+%i ucp)\n", num_fix_per_step,
             num_fix_per_step > 1 ? "s" : "",
             oldfreespin - freespin - num_fix_per_step);
      printStats();
    }
  }
  return -1;
}

bool reachedSolverThreshold(double average_magnetization) {
  return average_magnetization < PARAMAGNET;
}

// Find clauses which have only 1 variable, where the variable
// is currently unfrozen (spin=0), and fix it based on the
// variable's required value from the clause
void eliminateStandaloneClauses() {
  int c, var;
  for (c = 0; c < M; c++) {
    if (clause[c].lits == 1) {
      var = clause[c].literal[0].var;
      if (v[var].spin == 0) {
        printf("eliminating var %i (in 1-clause)\n", var);
        fix(var, clause[c].literal[0].bar ? -1 : 1);
      }
    }
  }
}

/************************************************************
 ************************************************************
 * VARIABLE SELECTION
 ************************************************************
 ************************************************************/

inline struct weightstruct normalize(struct weightstruct H) {
  // normalize a triplet
  double norm;
  norm = H.m + H.z + H.p; // minus, zero, plus
  H.m /= norm;
  H.p /= norm;
  H.z /= norm;
  return H;
}

int order(void const *a, void const *b) {
  // order relation for qsort, uses ranking in magn[]
  double aux;
  aux = magn[*((int *)b)] - magn[*((int *)a)];
  return aux < 0 ? -1 : (aux > 0 ? 1 : 0);
}

double index_biased(struct weightstruct H) {
  // most biased ranking
  return fabs(H.p - H.m);
}

double index_para(struct weightstruct H) {
  // least paramagnetic ranking
  return H.z;
}

// added
// Following our discussion, taking H.m as positive and H.p as negative
double abs_index_biased(struct weightstruct H) {
  // abs biased ranking
  return H.m - H.p;
}

double pos_index_biased(struct weightstruct H) {
  // abs biased ranking
  return H.m + H.z / 2;
}

double neg_index_biased(struct weightstruct H) {
  // abs biased ranking
  return H.p + H.z / 2;
}

void buildList(double (*index)(struct weightstruct), int *numInList,
               double *averageMagnetization) {
  // build an ordered list with order *index which is one of index_?
  int var;
  struct weightstruct H;
  double summag;
  double maxmag;
  summag = 0;
  *numInList = 0;
  for (var = 1; var <= N; var++) {
    if (v[var].spin == 0) {
      H = computeField(var);
      list[(*numInList)++] = var;
      magn[var] = (*index)(H);
      maxmag = H.p > H.m ? H.p : H.m;

      summag += maxmag;
    }
    if (v[var].spin == 0) {
      // added
      magn_sign[var] = abs_index_biased(H);
      pos_magn_sign[var] = pos_index_biased(H);
      neg_magn_sign[var] = neg_index_biased(H);
      free_magn_sign[var] = index_para(H);
    }
  }
  qsort(list, (*numInList), sizeof(int), &order);
  *averageMagnetization = summag / (*numInList);
}

void filterCandidateList(int numInList, std::vector<int> &out) {
  out.clear();
  for (int i = 0; i < numInList; i++) {
    int var = list[i];
    if (var <= 0 || var > N) continue;
    if (v[var].spin != 0) continue;
    if (use_streamlining && (v[var].artificialclauses > disjunction_limit))
      continue;
    out.push_back(var);
  }
}

int fixChunk(int numInList, int numToFix) {
  std::vector<int> candidates;
  filterCandidateList(numInList, candidates);

  std::cout << numToFix << " " << candidates.size() << std::endl;

  int last_var = std::min((size_t)(use_streamlining ? numToFix * 2 : numToFix),
                          candidates.size());

  for (int i = 0; i < last_var; i++) {
    int var1 = candidates[i];
    struct weightstruct H1 = computeField(var1);
    if (verbose > 1) {
      printf("H1(%i)={%f,%f,%f} ---> %s\n", var1, H1.p, H1.z, H1.m,
             H1.p > H1.m ? "-" : "+");
    }

    if (use_streamlining) {
      int var2_ind = last_var - 1 - i;
      if (var2_ind <= i) break;
      int var2 = candidates[var2_ind];

      struct weightstruct H2 = computeField(var2);
      double m1 = fabs(H1.p - H1.m);
      double m2 = fabs(H2.p - H2.m);

      if (verbose > 1) {
        printf("H2(%i)={%f,%f,%f} ---> %s\n", var2, H2.p, H2.z, H2.m,
               H2.p > H2.m ? "-" : "+");
      }

      if (verbose > 1) {
        std::cout << "OR constraining " << var1 << " (m=" << m1 << ") and "
                  << var1 << " (m=" << m2 << ")" << std::endl;
      }
      fixTwo(var1, var2, H1.p > H1.m ? -1 : 1, H2.p > H2.m ? -1 : 1);
    } else {
      fix(var1, H1.p > H1.m ? -1 : 1);
    }

    // Once we've fixed the last needed clause, break.
    if (numToFix-- == 1) break;
  }
  return numToFix;
}

/************************************************************
 ************************************************************
 * SURVEY PROPAGATION
 ************************************************************
 ************************************************************/

void randomizeEta() {
  // pick initial random values
  int i, j;
  for (i = 0; i < M; i++) {
    for (j = 0; j < clause[i].lits; j++) {
      clause[i].literal[j].eta = std::rand();
    }
  }
}

void initMemory() {
  // allocate mem (can be called more than once)
  delete[] perm;
  delete[] list;
  delete[] magn;
  delete[] prod;
  delete[] magn_sign;       // added
  delete[] pos_magn_sign;   // added
  delete[] neg_magn_sign;   // added
  delete[] free_magn_sign;  // added

  perm = new int[maxclauses];
  list = new int[N + 1];
  magn = new double[N + 1];
  prod = new double[maxlit];
  magn_sign = new double[N + 1];       // added
  pos_magn_sign = new double[N + 1];   // added
  neg_magn_sign = new double[N + 1];   // added
  free_magn_sign = new double[N + 1];  // added

  if (!perm || !list || !magn || !prod || !magn_sign || !pos_magn_sign ||
      !neg_magn_sign || !free_magn_sign) {
    fprintf(stderr, "Not enough memory for internal structures\n");
    exit(-1);
  }
}

void computePi() {
  // compute pi products of all vars from scratch
  int i, var;
  struct clauselist *cl;
  struct literalstruct *l;
  for (var = 1; var <= N; ++var) {
    if (v[var].spin == 0) {
      v[var].pi.p = 1;
      v[var].pi.m = 1;
      v[var].pi.pzero = 0;
      v[var].pi.mzero = 0;
      for (i = 0, cl = v[var].clauselist; i < v[var].clauses; i++, cl++) {
        if (cl->clause->type) {
          l = cl->clause->literal + cl->lit;
          if (l->bar) {
            if (1 - l->eta > EPS) {
              v[var].pi.p *= 1 - l->eta;
            } else {
              v[var].pi.pzero++;
            }
          } else {
            if (1 - l->eta > EPS) {
              v[var].pi.m *= 1 - l->eta;
            } else {
              v[var].pi.mzero++;
            }
          }
        }
      }
    }
  }
}

double updateEta(int cl) {
  // updates all eta's and pi's in clause cl
  int i;
  struct clausestruct *c;
  struct literalstruct *l;
  struct pistruct *pi;
  double eps;
  double neweta;
  double allprod = 1;
  double wt, wn;
  int zeroes = 0;
  double m, p;

  c = &(clause[cl]);
  for (i = 0, l = c->literal; i < c->lits; i++, l++)
    if (v[l->var].spin == 0) {
      pi = &(v[l->var].pi);
      if (l->bar) {
        m = pi->mzero ? 0 : pi->m;
        if (pi->pzero == 0) {
          p = pi->p / (1 - l->eta);
        } else if (pi->pzero == 1 && 1 - l->eta < EPS) {
          p = pi->p;
        } else {
          p = 0;
        }
        wn = p * (1 - m * norho);
        wt = m;
      } else {
        p = pi->pzero ? 0 : pi->p;
        if (pi->mzero == 0) {
          m = pi->m / (1 - l->eta);
        } else if (pi->mzero == 1 && 1 - l->eta < EPS) {
          m = pi->m;
        } else {
          m = 0;
        }
        wn = m * (1 - p * norho);
        wt = p;
      }
      prod[i] = wn / (wn + wt);
      if (prod[i] < EPS) {
        if (++zeroes == 2) break;
      } else {
        allprod *= prod[i];
      }
    }
  eps = 0;
  for (i = 0, l = c->literal; i < c->lits; i++, l++)
    if (v[l->var].spin == 0) {
      if (!zeroes) {
        neweta = allprod / prod[i];
      } else if (zeroes == 1 && prod[i] < EPS) {
        neweta = allprod;
      } else {
        neweta = 0;
      }

      pi = &(v[l->var].pi);
      if (l->bar) {
        if (1 - l->eta > EPS) {
          if (1 - neweta > EPS) {
            pi->p *= (1 - neweta) / (1 - l->eta);
          } else {
            pi->p /= (1 - l->eta);
            pi->pzero++;
          }
        } else {
          if (1 - neweta > EPS) {
            pi->p *= (1 - neweta);
            pi->pzero--;
          }
        }
      } else {
        if (1 - l->eta > EPS) {
          if (1 - neweta > EPS) {
            pi->m *= (1 - neweta) / (1 - l->eta);
          } else {
            pi->m /= (1 - l->eta);
            pi->mzero++;
          }
        } else {
          if (1 - neweta > EPS) {
            pi->m *= (1 - neweta);
            pi->mzero--;
          }
        }
      }
      if (eps < fabs(l->eta - neweta)) eps = fabs(fabs(l->eta - neweta));
      l->eta = neweta;
    }
  return eps;
}

struct weightstruct computeField(int var) {
  // compute H field of the variable var
  struct weightstruct H;
  double p, m;
  p = v[var].pi.pzero ? 0 : v[var].pi.p;
  m = v[var].pi.mzero ? 0 : v[var].pi.m;
  H.z = p * m;
  H.p = m - H.z;
  H.m = p - H.z;
  return normalize(H);
}

int runSurveyPropagation() {
  /* Run surveyprop iterations until convergence. */
  double eps = 0;
  int iter = 0, cl, quant;
  computePi();

  for (cl = 0, quant = 0; cl < M; cl++)
    if (clause[cl].type) {
      perm[quant++] = cl;
    }

  do {
    eps = surveyPropagationIteration();
    fflush(stderr);
  } while (eps > epsilon && iter++ < iterations);
  if (eps <= epsilon) {
    return 1;
  } else {
    printf("SP did NOT converge for t:%d\n", streamlining_iter);
    return 0;
  }
}

double surveyPropagationIteration() {
  int cl, vi = 0, quant, i;

  double eps, maxeps;
  eps = 0;
  maxeps = 0;
  for (quant = M - ncl[0]; quant; --quant) {
    cl = perm[i = (std::rand() % quant)];
    perm[i] = perm[quant - 1];
    perm[quant - 1] = cl;
    eps = updateEta(cl);
    if (eps > epsilon) {
      vi++;
    }
    if (eps > maxeps) {
      maxeps = eps;
    }
  }
  return maxeps;
}

}  // end namespace sp

}  // end namespace preimage
