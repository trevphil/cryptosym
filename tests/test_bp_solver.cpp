/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <gtest/gtest.h>

#include <memory>

#include "bp/bp_solver.hpp"
#include "core/solver.hpp"
#include "core/sym_hash.hpp"
#include "eval_solver.hpp"
#include "hashing/sym_md5.hpp"
#include "hashing/sym_ripemd160.hpp"
#include "hashing/sym_sha256.hpp"

namespace preimage {

TEST(BPSolverTest, SolveSHA256) {
  std::shared_ptr<Solver> solver = std::make_shared<bp::BPSolver>();
  for (int d = 1; d <= 2; ++d) {
    std::shared_ptr<SymHash> hasher = std::make_shared<SHA256>(64, d);
    for (int i = 0; i < 3; ++i) {
      const bool solved = evaluateSolver(solver, hasher);
      EXPECT_TRUE(solved);
    }
  }
}

TEST(BPSolverTest, SolveMD5) {
  std::shared_ptr<Solver> solver = std::make_shared<bp::BPSolver>();
  for (int d = 1; d <= 3; ++d) {
    std::shared_ptr<SymHash> hasher = std::make_shared<MD5>(64, d);
    for (int i = 0; i < 3; ++i) {
      const bool solved = evaluateSolver(solver, hasher);
      EXPECT_TRUE(solved);
    }
  }
}

TEST(BPSolverTest, SolveRIPEMD160) {
  std::shared_ptr<Solver> solver = std::make_shared<bp::BPSolver>();
  for (int d = 1; d <= 3; ++d) {
    std::shared_ptr<SymHash> hasher = std::make_shared<RIPEMD160>(64, d);
    for (int i = 0; i < 3; ++i) {
      const bool solved = evaluateSolver(solver, hasher);
      EXPECT_TRUE(solved);
    }
  }
}

TEST(BPSolverTest, NegatedBitAssignments) {
  const LogicGate g = LogicGate::fromString("A 3 1 2");
  const SymRepresentation problem({g}, {1, 2}, {3});
  std::unordered_map<int, bool> assignments;
  assignments[3] = true;
  assignments[-2] = false;
  bp::BPSolver solver;
  EXPECT_THROW({ solver.solve(problem, assignments); }, std::invalid_argument);
}

}  // end namespace preimage
