/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

#include "core/logic_gate.hpp"

namespace preimage {

using GType = LogicGate::Type;

bool isSAT(const std::vector<std::vector<int>> &clauses,
           const std::map<int, bool> &vars) {
  for (const std::vector<int> &clause : clauses) {
    bool is_sat = false;
    for (int lit : clause) {
      const bool truth_val = lit < 0 ? !vars.at(-lit) : vars.at(lit);
      if (truth_val) {
        is_sat = true;
        break;
      }
    }
    if (!is_sat) return false;
  }
  return true;
}

TEST(LogicGateTest, Initialization) {
  const LogicGate g(GType::and_gate, 3, {1, 2});
  EXPECT_EQ(g.t(), GType::and_gate);
  EXPECT_EQ(g.output, 3);
  EXPECT_EQ(g.inputs.at(0), 1);
  EXPECT_EQ(g.inputs.at(1), 2);
}

TEST(LogicGateTest, StringConversion) {
  const std::string s = "M 4 -1 2 -3";
  const LogicGate g = LogicGate::fromString(s);
  EXPECT_EQ(g.t(), GType::maj_gate);
  EXPECT_EQ(g.output, 4);
  EXPECT_EQ(g.inputs.at(0), -1);
  EXPECT_EQ(g.inputs.at(1), 2);
  EXPECT_EQ(g.inputs.at(2), -3);
  EXPECT_EQ(g.toString(), s);
}

TEST(LogicGateTest, AndGateCNF) {
  const LogicGate g(GType::and_gate, 3, {1, 2});
  const auto clauses = g.cnf();
  for (int i = 0; i < (1 << 3); ++i) {
    const bool b1 = (i >> 0) & 1;
    const bool b2 = (i >> 1) & 1;
    const bool b3 = (i >> 2) & 1;
    const bool expected = b1 & b2;
    if (b3 == expected) {
      EXPECT_TRUE(isSAT(clauses, {{1, b1}, {2, b2}, {3, b3}}));
    } else {
      EXPECT_FALSE(isSAT(clauses, {{1, b1}, {2, b2}, {3, b3}}));
    }
  }
}

TEST(LogicGateTest, OrGateCNF) {
  const LogicGate g(GType::or_gate, 3, {1, 2});
  const auto clauses = g.cnf();
  for (int i = 0; i < (1 << 3); ++i) {
    const bool b1 = (i >> 0) & 1;
    const bool b2 = (i >> 1) & 1;
    const bool b3 = (i >> 2) & 1;
    const bool expected = b1 | b2;
    if (b3 == expected) {
      EXPECT_TRUE(isSAT(clauses, {{1, b1}, {2, b2}, {3, b3}}));
    } else {
      EXPECT_FALSE(isSAT(clauses, {{1, b1}, {2, b2}, {3, b3}}));
    }
  }
}

TEST(LogicGateTest, XorGateCNF) {
  const LogicGate g(GType::xor_gate, 3, {1, 2});
  const auto clauses = g.cnf();
  for (int i = 0; i < (1 << 3); ++i) {
    const bool b1 = (i >> 0) & 1;
    const bool b2 = (i >> 1) & 1;
    const bool b3 = (i >> 2) & 1;
    const bool expected = b1 ^ b2;
    if (b3 == expected) {
      EXPECT_TRUE(isSAT(clauses, {{1, b1}, {2, b2}, {3, b3}}));
    } else {
      EXPECT_FALSE(isSAT(clauses, {{1, b1}, {2, b2}, {3, b3}}));
    }
  }
}

TEST(LogicGateTest, Xor3GateCNF) {
  const LogicGate g(GType::xor3_gate, 4, {1, 2, 3});
  const auto clauses = g.cnf();
  for (int i = 0; i < (1 << 4); ++i) {
    const bool b1 = (i >> 0) & 1;
    const bool b2 = (i >> 1) & 1;
    const bool b3 = (i >> 2) & 1;
    const bool b4 = (i >> 3) & 1;
    const bool expected = b1 ^ b2 ^ b3;
    if (b4 == expected) {
      EXPECT_TRUE(isSAT(clauses, {{1, b1}, {2, b2}, {3, b3}, {4, b4}}));
    } else {
      EXPECT_FALSE(isSAT(clauses, {{1, b1}, {2, b2}, {3, b3}, {4, b4}}));
    }
  }
}

TEST(LogicGateTest, Maj3GateCNF) {
  const LogicGate g(GType::maj_gate, 4, {1, 2, 3});
  const auto clauses = g.cnf();
  for (int i = 0; i < (1 << 4); ++i) {
    const bool b1 = (i >> 0) & 1;
    const bool b2 = (i >> 1) & 1;
    const bool b3 = (i >> 2) & 1;
    const bool b4 = (i >> 3) & 1;
    const bool expected = (int(b1) + int(b2) + int(b3)) > 1;
    if (b4 == expected) {
      EXPECT_TRUE(isSAT(clauses, {{1, b1}, {2, b2}, {3, b3}, {4, b4}}));
    } else {
      EXPECT_FALSE(isSAT(clauses, {{1, b1}, {2, b2}, {3, b3}, {4, b4}}));
    }
  }
}

TEST(LogicGateTest, WrongNumberOfInputs) {
  EXPECT_THROW({ LogicGate(GType::and_gate, 2, {1}); }, std::invalid_argument);
  EXPECT_THROW({ LogicGate(GType::and_gate, 4, {1, 2, 3}); }, std::invalid_argument);

  EXPECT_THROW({ LogicGate(GType::or_gate, 2, {1}); }, std::invalid_argument);
  EXPECT_THROW({ LogicGate(GType::or_gate, 4, {1, 2, 3}); }, std::invalid_argument);

  EXPECT_THROW({ LogicGate(GType::xor_gate, 2, {1}); }, std::invalid_argument);
  EXPECT_THROW({ LogicGate(GType::xor_gate, 4, {1, 2, 3}); }, std::invalid_argument);

  EXPECT_THROW({ LogicGate(GType::xor3_gate, 3, {1, 2}); }, std::invalid_argument);
  EXPECT_THROW({ LogicGate(GType::xor3_gate, 5, {1, 2, 3, 4}); }, std::invalid_argument);

  EXPECT_THROW({ LogicGate(GType::maj_gate, 3, {1, 2}); }, std::invalid_argument);
  EXPECT_THROW({ LogicGate(GType::maj_gate, 5, {1, 2, 3, 4}); }, std::invalid_argument);
}

TEST(LogicGateTest, NegatedOutput) {
  EXPECT_THROW({ LogicGate(GType::xor_gate, -3, {1, 2}); }, std::invalid_argument);
}

TEST(LogicGateTest, ZeroIndexedVariables) {
  EXPECT_THROW({ LogicGate(GType::or_gate, 0, {1, 2}); }, std::invalid_argument);
  EXPECT_THROW({ LogicGate(GType::or_gate, 2, {0, 1}); }, std::invalid_argument);
}

}  // end namespace preimage
