/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <gtest/gtest.h>

#include <set>
#include <unordered_map>

#include "core/cnf.hpp"

namespace preimage {

TEST(CNFTest, Empty) {
  CNF cnf;
  EXPECT_EQ(cnf.num_vars, 0);
  EXPECT_EQ(cnf.num_clauses, 0);
}

TEST(CNFTest, Initialization) {
  CNF cnf({{1, 2, -3}, {-2, 4}}, 4);
  EXPECT_EQ(cnf.num_vars, 4);
  EXPECT_EQ(cnf.num_clauses, 2);
}

TEST(CNFTest, Simplification) {
  CNF cnf({{1, 2, -3}, {-2, 4}}, 4);
  std::unordered_map<int, bool> assignments;
  assignments[2] = false;
  cnf = cnf.simplify(assignments);
  EXPECT_EQ(cnf.num_clauses, 1);
  EXPECT_EQ(cnf.num_vars, 2);
  const std::set<int> clause = {-1, 2};
  EXPECT_EQ(cnf.clauses[0], clause);

  assignments.clear();
  assignments[1] = false;
  cnf = cnf.simplify(assignments);
  EXPECT_EQ(cnf.num_clauses, 0);
  EXPECT_EQ(cnf.num_vars, 0);
}

TEST(CNFTest, ApproximationRatio) {
  CNF cnf({{-1, 2}, {3, -4}, {-5, 6}, {7, -8}}, 8);
  std::unordered_map<int, bool> assignments;
  assignments[1] = true;
  assignments[2] = false;
  assignments[3] = false;
  assignments[4] = true;
  assignments[5] = true;
  assignments[6] = false;
  assignments[7] = false;
  assignments[8] = false;
  EXPECT_EQ(cnf.numSatClauses(assignments), 1);
  EXPECT_DOUBLE_EQ(cnf.approximationRatio(assignments), 0.25);
}

TEST(CNFTest, ReadWrite) {
  CNF cnf({{-1, 2}, {3, -4}}, 8);
  cnf.toFile("/tmp/a.cnf");
  cnf = CNF::fromFile("/tmp/a.cnf");
  EXPECT_EQ(cnf.num_vars, 8);
  EXPECT_EQ(cnf.num_clauses, 2);
  const std::set<int> clause0 = {-1, 2};
  const std::set<int> clause1 = {-4, 3};
  EXPECT_EQ(cnf.clauses[0], clause0);
  EXPECT_EQ(cnf.clauses[1], clause1);
}

}  // end namespace preimage
