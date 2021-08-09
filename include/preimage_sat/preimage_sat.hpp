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

#include <spdlog/spdlog.h>

#include <set>
#include <queue>
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>

#include "core/solver.hpp"
#include "core/utils.hpp"

namespace preimage {

class PreimageSATSolver : public Solver {
 public:
  struct LitStats {
    LitStats() {}
    LitStats(int i, bool a) : lit(i), preferred_assignment(a) {}

    int lit;
    bool preferred_assignment;
    int num_referenced_gates;

    double score() const {
      return static_cast<double>(num_referenced_gates);
    }
  };

  struct StackItem {
    StackItem() {}
    StackItem(int lit) : lit_guess(lit), implied({}), second_try(false) {}
    int lit_guess;
    std::set<int> implied;
    bool second_try;
  };

  explicit PreimageSATSolver(bool verbose);

  virtual ~PreimageSATSolver();

  std::string solverName() const override { return "PreimageSAT"; }

 protected:
  void initialize() override;

  std::unordered_map<int, bool> solveInternal() override;

 private:
  LitStats computeStats(const int lit);

  void pushStack(int lit, bool truth_value, bool second_try);

  bool popStack(int &lit, bool &truth_value);

  bool popStack();

  int pickLiteral(bool &assignment);

  int propagate(const int lit);

  inline bool getLitValue(const int lit) const {
    return lit < 0 ? (literals[-lit] < 0) : (literals[lit] > 0);
  }

  inline void setLitValue(const int lit, const bool val) {
    if (lit < 0) {
      literals[-lit] = val ? -1 : 1;
    } else {
      literals[lit] = val ? 1 : -1;
    }
  }

  bool partialSolve(const LogicGate &g,
                    std::vector<int> &solved_lits);

  bool partialSolveAnd(const LogicGate &g,
                       std::vector<int> &solved_lits);

  bool partialSolveOr(const LogicGate &g,
                      std::vector<int> &solved_lits);

  bool partialSolveXor(const LogicGate &g,
                       std::vector<int> &solved_lits);

  bool partialSolveXor3(const LogicGate &g,
                        std::vector<int> &solved_lits);

  bool partialSolveMaj(const LogicGate &g,
                       std::vector<int> &solved_lits);

  void setupVisualization();

  void renderDebugImage();

  std::vector<int8_t> literals;
  std::vector<StackItem> stack;
  std::vector<LitStats> literal_ordering;
  std::unordered_map<int, std::set<int>> lit2gates;

  bool visualize;
  cv::Mat debug_image;
  std::vector<double> lit_pos_x;
  std::vector<double> lit_pos_y;
};

}  // end namespace preimage