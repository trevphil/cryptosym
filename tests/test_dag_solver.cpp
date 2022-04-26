/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <gtest/gtest.h>

#include <memory>

#include "core/solver.hpp"
#include "core/sym_hash.hpp"
#include "dag_solver/dag_solver.hpp"
#include "eval_solver.hpp"
#include "hashing/sym_md5.hpp"
#include "hashing/sym_ripemd160.hpp"
#include "hashing/sym_sha256.hpp"

namespace preimage {

TEST(DAGSolverTest, SolveSHA256) {
  std::shared_ptr<Solver> solver = std::make_shared<DAGSolver>();
  for (int d = 4; d <= 8; ++d) {
    std::shared_ptr<SymHash> hasher = std::make_shared<SHA256>(64, d);
    for (int i = 0; i < 5; ++i) {
      const bool solved = evaluateSolver(solver, hasher);
      EXPECT_TRUE(solved);
    }
  }
}

TEST(DAGSolverTest, SolveMD5) {
  std::shared_ptr<Solver> solver = std::make_shared<DAGSolver>();
  for (int d = 8; d <= 12; ++d) {
    std::shared_ptr<SymHash> hasher = std::make_shared<MD5>(64, d);
    for (int i = 0; i < 5; ++i) {
      const bool solved = evaluateSolver(solver, hasher);
      EXPECT_TRUE(solved);
    }
  }
}

TEST(DAGSolverTest, SolveRIPEMD160) {
  std::shared_ptr<Solver> solver = std::make_shared<DAGSolver>();
  for (int d = 8; d <= 12; ++d) {
    std::shared_ptr<SymHash> hasher = std::make_shared<RIPEMD160>(64, d);
    for (int i = 0; i < 5; ++i) {
      const bool solved = evaluateSolver(solver, hasher);
      EXPECT_TRUE(solved);
    }
  }
}

}  // end namespace preimage