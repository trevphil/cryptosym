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
#include <vector>

#include "factor.hpp"
#include "bit.hpp"

namespace preimage {

class Solver {
 public:
  Solver(const std::map<size_t, Factor> &factors,
         const std::vector<size_t> &input_indices);

  virtual ~Solver();

  std::map<size_t, bool> solve(const std::map<size_t, bool> &observed);

 protected:
  void reset();

  virtual std::map<size_t, bool> solveInternal();

  std::map<size_t, Factor> factors_;
  std::map<size_t, bool> observed_;
  std::vector<size_t> input_indices_;

 private:
  void setImplicitObserved();
  size_t propagateBackward();
  void propagateForward(size_t smallest_obs);
};

}  // end namespace preimage
