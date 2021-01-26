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

namespace dataset_generator {

class Solver {
 public:
  Solver(const std::vector<Factor> &factors,
         const std::vector<size_t> &input_indices);

  virtual ~Solver();

  virtual std::map<size_t, bool> solve(const std::map<size_t, bool> &observed);

 protected:
  void reset();

  std::vector<Factor> factors_;
  std::map<size_t, bool> observed_;
  std::vector<size_t> input_indices_;

 private:
  void setImplicitObserved();
  void propagateBackward();
  void propagateForward();
};

}  // end namespace dataset_generator
