/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include <gtest/gtest.h>

#include <memory>

#include "core/logic_gate.hpp"
#include "core/solver.hpp"
#include "core/sym_hash.hpp"
#include "eval_solver.hpp"
#include "hashing/sym_md5.hpp"
#include "hashing/sym_ripemd160.hpp"
#include "hashing/sym_sha256.hpp"
#include "sdp/sdp_solver.hpp"

namespace preimage {

TEST(SDPSolverTest, SolveSHA256) {
  std::shared_ptr<Solver> solver = std::make_shared<SDPSolver>();
  std::shared_ptr<SymHash> hasher = std::make_shared<SymSHA256>(64, 16);
  for (int i = 0; i < 5; ++i) {
    const bool solved = evaluateSolver(solver, hasher);
    EXPECT_TRUE(solved);
  }
}

TEST(SDPSolverTest, SolveMD5) {
  std::shared_ptr<Solver> solver = std::make_shared<SDPSolver>();
  std::shared_ptr<SymHash> hasher = std::make_shared<SymMD5>(64, 16);
  for (int i = 0; i < 5; ++i) {
    const bool solved = evaluateSolver(solver, hasher);
    EXPECT_TRUE(solved);
  }
}

TEST(SDPSolverTest, SolveRIPEMD160) {
  std::shared_ptr<Solver> solver = std::make_shared<SDPSolver>();
  for (int d = 8; d <= 12; ++d) {
    std::shared_ptr<SymHash> hasher = std::make_shared<SymRIPEMD160>(64, d);
    for (int i = 0; i < 5; ++i) {
      const bool solved = evaluateSolver(solver, hasher);
      EXPECT_TRUE(solved);
    }
  }
}

TEST(SDPSolverTest, NegatedBitAssignments) {
  const LogicGate g = LogicGate::fromString("A 3 1 -2");
  const SymRepresentation problem({g}, {1, -2}, {3});
  std::unordered_map<int, bool> assignments;
  assignments[3] = true;
  assignments[-2] = true;
  SDPSolver solver;
  EXPECT_THROW({ solver.solve(problem, assignments); }, std::invalid_argument);
}

TEST(SDPSolverTest, UnsatisfiableProblem) {
  const LogicGate g = LogicGate::fromString("A 3 1 -2");
  const SymRepresentation problem({g}, {1, -2}, {3});
  std::unordered_map<int, bool> assignments;
  assignments[3] = true;
  assignments[1] = true;
  assignments[2] = true;
  SDPSolver solver;
  EXPECT_THROW({ solver.solve(problem, assignments); }, std::runtime_error);
}

TEST(SDPSolverTest, ApproximateSolveMD5) {
  SDPSolver sdp(100);
  SymMD5 md5(64, 20);
  const SymRepresentation problem = md5.getSymbolicRepresentation();
  for (int i = 0; i < 4; ++i) {
    const BitVec hash_out = md5.callRandom();
    const auto solution = sdp.solve(problem, hash_out);
    const CNF cnf = problem.toCNF();
    const double approx_ratio = cnf.approximationRatio(solution);
    EXPECT_GE(approx_ratio, 0.92);
  }
}

}  // end namespace preimage
