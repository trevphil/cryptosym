/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include <gtest/gtest.h>

#include <memory>

#include "cmsat/cmsat_solver.hpp"
#include "core/logic_gate.hpp"
#include "core/solver.hpp"
#include "core/sym_hash.hpp"
#include "eval_solver.hpp"
#include "hashing/sym_md5.hpp"
#include "hashing/sym_ripemd160.hpp"
#include "hashing/sym_sha256.hpp"

namespace preimage {

TEST(CMSatSolverTest, SolveSHA256) {
  std::shared_ptr<Solver> solver = std::make_shared<CMSatSolver>();
  for (int d = 4; d <= 8; ++d) {
    std::shared_ptr<SymHash> hasher = std::make_shared<SymSHA256>(64, d);
    for (int i = 0; i < 5; ++i) {
      const bool solved = evaluateSolver(solver, hasher);
      EXPECT_TRUE(solved);
    }
  }
}

TEST(CMSatSolverTest, SolveMD5) {
  std::shared_ptr<Solver> solver = std::make_shared<CMSatSolver>();
  for (int d = 8; d <= 12; ++d) {
    std::shared_ptr<SymHash> hasher = std::make_shared<SymMD5>(64, d);
    for (int i = 0; i < 5; ++i) {
      const bool solved = evaluateSolver(solver, hasher);
      EXPECT_TRUE(solved);
    }
  }
}

TEST(CMSatSolverTest, SolveRIPEMD160) {
  std::shared_ptr<Solver> solver = std::make_shared<CMSatSolver>();
  for (int d = 8; d <= 12; ++d) {
    std::shared_ptr<SymHash> hasher = std::make_shared<SymRIPEMD160>(64, d);
    for (int i = 0; i < 5; ++i) {
      const bool solved = evaluateSolver(solver, hasher);
      EXPECT_TRUE(solved);
    }
  }
}

TEST(CMSatSolverTest, NegatedBitAssignments) {
  const LogicGate g = LogicGate::fromString("A 3 1 2");
  const SymRepresentation problem({g}, {1, 2}, {3});
  std::unordered_map<int, bool> assignments;
  assignments[3] = true;
  assignments[-2] = false;
  CMSatSolver solver;
  EXPECT_THROW({ solver.solve(problem, assignments); }, std::invalid_argument);
}

TEST(CMSatSolverTest, UnsatisfiableProblem) {
  const LogicGate g = LogicGate::fromString("A 3 1 -2");
  const SymRepresentation problem({g}, {1, -2}, {3});
  std::unordered_map<int, bool> assignments;
  assignments[3] = true;
  assignments[1] = true;
  assignments[2] = true;
  CMSatSolver solver;
  EXPECT_THROW({ solver.solve(problem, assignments); }, std::runtime_error);
}

}  // end namespace preimage
