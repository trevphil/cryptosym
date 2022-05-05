/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <gtest/gtest.h>

#include <vector>

#include "core/logic_gate.hpp"
#include "core/sym_representation.hpp"

namespace preimage {

TEST(SymRepresentationTest, Initialization) {
  const std::vector<LogicGate> gates = {
      LogicGate(LogicGate::Type::and_gate, 4, {1, -2}),
      LogicGate(LogicGate::Type::xor_gate, 5, {2, -3})};
  SymRepresentation rep(gates, {1, 2, 3}, {0, 4, 5});
  EXPECT_EQ(rep.numVars(), 5);
  EXPECT_EQ(rep.gates().size(), 2);
  EXPECT_EQ(rep.gates().at(1).t(), LogicGate::Type::and_gate);
  EXPECT_EQ(rep.gates().at(0).t(), LogicGate::Type::xor_gate);

  const std::vector<int> inputs = {1, 2, 3};
  const std::vector<int> outputs = {0, 4, 5};
  EXPECT_EQ(rep.inputIndices(), inputs);
  EXPECT_EQ(rep.outputIndices(), outputs);
}

TEST(SymRepresentationTest, PruneAndReindex) {
  const std::vector<LogicGate> gates = {
      LogicGate(LogicGate::Type::and_gate, 4, {1, -2}),
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
  const std::vector<LogicGate> gates = {
      LogicGate(LogicGate::Type::and_gate, 4, {1, -2}),
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
      LogicGate(LogicGate::Type::maj_gate, 6, {1, 4, 5})};
  const std::vector<int> inputs = {1, 2, 3};
  const std::vector<int> outputs = {0, 6, 5, 0, 0};
  SymRepresentation rep(gates, inputs, outputs);
  EXPECT_EQ(rep.numVars(), 6);
  EXPECT_EQ(rep.gates().size(), 3);
  EXPECT_EQ(rep.inputIndices(), inputs);
  EXPECT_EQ(rep.outputIndices(), outputs);

  rep.toDAG("/tmp/dag.txt");
  rep = SymRepresentation::fromDAG("/tmp/dag.txt");
  EXPECT_EQ(rep.numVars(), 6);
  EXPECT_EQ(rep.gates().size(), 3);
  EXPECT_EQ(rep.inputIndices(), inputs);
  EXPECT_EQ(rep.outputIndices(), outputs);
}

}  // end namespace preimage