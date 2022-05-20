/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <vector>

#include "core/logic_gate.hpp"
#include "core/sym_representation.hpp"

namespace preimage {

TEST(SymRepresentationTest, Initialization) {
  const std::vector<LogicGate> gates = {LogicGate(LogicGate::Type::and_gate, 4, {1, -2}),
                                        LogicGate(LogicGate::Type::xor_gate, 5, {2, -3})};
  SymRepresentation rep(gates, {1, 2, 3}, {0, 4, 5});
  EXPECT_EQ(rep.numVars(), 5);
  EXPECT_EQ(rep.gates().size(), 2);

  const std::vector<int> inputs = {1, 2, 3};
  const std::vector<int> outputs = {0, 4, 5};
  EXPECT_EQ(rep.inputIndices(), inputs);
  EXPECT_EQ(rep.outputIndices(), outputs);
}

TEST(SymRepresentationTest, PruneAndReindex) {
  const std::vector<LogicGate> gates = {LogicGate(LogicGate::Type::and_gate, 4, {1, -2}),
                                        LogicGate(LogicGate::Type::and_gate, 5, {3, -4})};
  SymRepresentation rep(gates, {1, 2, 3}, {4});
  EXPECT_EQ(rep.numVars(), 3);
  EXPECT_EQ(rep.gates().size(), 1);
  EXPECT_EQ(rep.gates().at(0).t(), LogicGate::Type::and_gate);

  const std::vector<int> inputs = {1, 2, 0};
  const std::vector<int> outputs = {3};
  EXPECT_EQ(rep.inputIndices(), inputs);
  EXPECT_EQ(rep.outputIndices(), outputs);
}

TEST(SymRepresentationTest, ConvertToCNF) {
  const std::vector<LogicGate> gates = {LogicGate(LogicGate::Type::and_gate, 4, {1, -2}),
                                        LogicGate(LogicGate::Type::and_gate, 5, {3, -4})};
  SymRepresentation rep(gates, {1, 2, 3}, {4});
  const CNF cnf = rep.toCNF();
  EXPECT_EQ(cnf.num_vars, 3);
  EXPECT_EQ(cnf.num_clauses, 3);
}

TEST(SymRepresentationTest, ConvertDAG) {
  const std::vector<LogicGate> gates = {
      LogicGate(LogicGate::Type::and_gate, 4, {1, -2}),
      LogicGate(LogicGate::Type::xor_gate, 5, {2, -3}),
      LogicGate(LogicGate::Type::maj3_gate, 6, {1, 4, 5})};
  const std::vector<int> inputs = {1, 2, 3};
  const std::vector<int> outputs = {0, 6, 5, 0, 0};
  SymRepresentation rep(gates, inputs, outputs);
  EXPECT_EQ(rep.numVars(), 6);
  EXPECT_EQ(rep.gates().size(), 3);
  EXPECT_EQ(rep.inputIndices(), inputs);
  EXPECT_EQ(rep.outputIndices(), outputs);

  const std::filesystem::path path = std::filesystem::temp_directory_path() / "dag.txt";
  rep.toDAG(path.string());
  rep = SymRepresentation::fromDAG(path.string());
  EXPECT_EQ(rep.numVars(), 6);
  EXPECT_EQ(rep.gates().size(), 3);
  EXPECT_EQ(rep.inputIndices(), inputs);
  EXPECT_EQ(rep.outputIndices(), outputs);
}

TEST(SymRepresentationTest, LoadInvalidDAG) {
  std::filesystem::path path = std::filesystem::temp_directory_path();
  path = path / "not" / "a" / "dag.txt";
  EXPECT_ANY_THROW({ SymRepresentation::fromDAG(path.string()); });
}

}  // end namespace preimage
