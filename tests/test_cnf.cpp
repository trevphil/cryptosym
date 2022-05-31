/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
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
  std::unordered_map<int, int> lit_new_to_old;
  assignments[2] = false;
  cnf = cnf.simplify(assignments, lit_new_to_old);
  EXPECT_EQ(cnf.num_clauses, 1);
  EXPECT_EQ(cnf.num_vars, 2);
  const std::set<int> clause = {-1, 2};
  EXPECT_EQ(cnf.clauses[0], clause);
  EXPECT_EQ(assignments[2], false);
  EXPECT_EQ(lit_new_to_old.size(), 2);
  EXPECT_EQ(lit_new_to_old[1], 3);
  EXPECT_EQ(lit_new_to_old[2], 1);

  assignments.clear();
  lit_new_to_old.clear();
  assignments[2] = false;
  cnf = cnf.simplify(assignments, lit_new_to_old);
  EXPECT_EQ(cnf.num_clauses, 0);
  EXPECT_EQ(cnf.num_vars, 0);
  EXPECT_EQ(lit_new_to_old.size(), 0);
  EXPECT_EQ(assignments[2], false);
  EXPECT_EQ(assignments[1], false);
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

TEST(CNFTest, NumSatisfiedClausesPartialAssignment) {
  CNF cnf({{-1, 2}, {3, -4}}, 4);
  std::unordered_map<int, bool> assignments;
  assignments[-1] = true;
  assignments[3] = false;
  EXPECT_THROW({ cnf.numSatClauses(assignments); }, std::out_of_range);
}

TEST(CNFTest, ReadWrite) {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "a.cnf";
  CNF cnf({{-1, 2}, {3, -4}}, 8);
  cnf.toFile(path.string());
  cnf = CNF::fromFile(path.string());
  EXPECT_EQ(cnf.num_vars, 8);
  EXPECT_EQ(cnf.num_clauses, 2);
  const std::set<int> clause0 = {-1, 2};
  const std::set<int> clause1 = {-4, 3};
  EXPECT_EQ(cnf.clauses[0], clause0);
  EXPECT_EQ(cnf.clauses[1], clause1);
}

TEST(CNFTest, TrimSpaces) {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "whitespace.cnf";
  std::ofstream cnf_file;
  cnf_file.open(path.string());
  cnf_file << "# This is a comment \n";
  cnf_file << " # Also a comment\n";
  cnf_file << "p cnf 3 2 \r\n";
  cnf_file << "# Another comment\n";
  cnf_file << " 2 -1    3\t0 \r\n";
  cnf_file << "\t-3 2 0\n";
  cnf_file << "\n";
  cnf_file.close();
  CNF cnf = CNF::fromFile(path.string());
  EXPECT_EQ(cnf.num_vars, 3);
  EXPECT_EQ(cnf.num_clauses, 2);
  const std::set<int> clause0 = {-1, 2, 3};
  const std::set<int> clause1 = {-3, 2};
  EXPECT_EQ(cnf.clauses[0], clause0);
  EXPECT_EQ(cnf.clauses[1], clause1);
}

TEST(CNFTest, LoadFromNonexistantFile) {
  std::filesystem::path path = std::filesystem::temp_directory_path();
  path = path / "not" / "a" / "file.cnf";
  EXPECT_THROW({ CNF::fromFile(path.string()); }, std::invalid_argument);
}

TEST(CNFTest, LoadDimacsWithoutHeader) {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "dimacs.cnf";
  std::fstream dimacs;
  dimacs.open(path.string(), std::ios_base::out);
  ASSERT_TRUE(dimacs.is_open());
  dimacs << "1 2 3 0\n";
  dimacs << "3 -1 -4 0\n";
  dimacs.close();
  EXPECT_THROW({ CNF::fromFile(path.string()); }, std::runtime_error);
}

TEST(CNFTest, SimplifyWithZeroIndexedAssignments) {
  CNF cnf({{-1, 2}, {3, -4}}, 8);
  std::unordered_map<int, bool> assignments;
  std::unordered_map<int, int> lit_new_to_old;
  assignments[2] = true;
  assignments[0] = false;
  EXPECT_THROW({ cnf.simplify(assignments, lit_new_to_old); }, std::invalid_argument);
}

TEST(CNFTest, SimplifyWithNegativeIndexedAssignments) {
  CNF cnf({{-1, 2}, {3, -4}}, 8);
  std::unordered_map<int, bool> assignments;
  std::unordered_map<int, int> lit_new_to_old;
  assignments[-2] = false;
  EXPECT_THROW({ cnf.simplify(assignments, lit_new_to_old); }, std::invalid_argument);
}

TEST(CNFTest, SimplifyResultsInUnsatisfiability) {
  CNF cnf({{-1, 2}, {-2, 3}}, 3);
  std::unordered_map<int, bool> assignments;
  std::unordered_map<int, int> lit_new_to_old;
  assignments[1] = true;
  assignments[3] = false;
  EXPECT_THROW({ cnf.simplify(assignments, lit_new_to_old); }, std::runtime_error);
}

}  // end namespace preimage
