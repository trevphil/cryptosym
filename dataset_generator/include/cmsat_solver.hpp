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
#include <cryptominisat5/cryptominisat.h>

#include "solver.hpp"

namespace dataset_generator {

class CMSatSolver : public Solver {
 public:
  CMSatSolver(const std::vector<Factor> &factors,
              const std::vector<size_t> &input_indices);

  std::map<size_t, bool> solve(const std::map<size_t, bool> &observed) override;

 private:
  CMSat::SATSolver* solver_;
  std::map<size_t, unsigned int> rv2idx_;
};

}  // end namespace dataset_generator
