/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "core/logic_gate.hpp"

namespace preimage {

class Solver {
 public:
  Solver();

  virtual ~Solver();

  void setHeader(int inp_size, int out_size, int n_vars, int n_gates);

  void setInputIndices(const std::vector<int> &input_indices);

  void setOutputIndices(const std::vector<int> &output_indices);

  void setGates(const std::vector<LogicGate> &gates);

  void setObserved(const std::unordered_map<int, bool> &observed);

  std::unordered_map<int, bool> solve();

  virtual std::string solverName() const = 0;

 protected:
  virtual void initialize() = 0;

  virtual std::unordered_map<int, bool> solveInternal() = 0;

  int input_size_, output_size_;
  int num_vars_, num_gates_;
  std::vector<LogicGate> gates_;
  std::vector<int> input_indices_, output_indices_;
  std::unordered_map<int, bool> observed_;
};

}  // end namespace preimage
