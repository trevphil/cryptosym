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

namespace preimage {

namespace sp {

int run(std::map<int, bool> &solution);
bool reachedSolverThreshold(double average_magnetization);
void eliminateStandaloneClauses();
double surveyPropagationIteration();
void randomizeEta();
void initMemory();
double updateEta(int cl);
void computePi();
struct weightstruct computeField(int var);
int runSurveyPropagation();
int fix(int var, int spin);
int fixChunk(int numInList, int quant);
void buildList(double (*)(struct weightstruct), int *, double *);
double index_biased(struct weightstruct H);
double index_para(struct weightstruct H);
double abs_index_biased(struct weightstruct H);
double pos_index_biased(struct weightstruct H);
double neg_index_biased(struct weightstruct H);

}  // end namespace sp

}  // end namespace preimage
