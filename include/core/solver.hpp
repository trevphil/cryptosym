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
#include <string>

#include "core/factor.hpp"
#include "core/bit.hpp"

namespace preimage {

class Solver {
 public:
  explicit Solver(bool verbose);

  virtual ~Solver();

  void setFactors(const std::map<int, Factor> &factors);

  void setInputIndices(const std::vector<int> &input_indices);

  void setObserved(const std::map<int, bool> &observed);

  std::map<int, bool> solve();

  virtual std::string solverName() const = 0;

 protected:
  virtual void initialize() = 0;

  virtual std::map<int, bool> solveInternal() = 0;

  bool verbose_;
  std::map<int, Factor> factors_;
  std::vector<int> input_indices_;
  std::map<int, bool> observed_;

 private:
  void setImplicitObserved();
  int propagateBackward();
  void propagateForward(int smallest_obs);
};

}  // end namespace preimage
