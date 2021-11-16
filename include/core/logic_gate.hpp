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

#include <string>
#include <vector>

namespace preimage {

class LogicGate {
 public:
  enum Type : char {
    and_gate = 'A',
    xor_gate = 'X',
    or_gate = 'O',
    maj_gate = 'M',
    xor3_gate = 'Z'
  };

  LogicGate();

  LogicGate(Type typ, const int dpth, const int output,
            const std::vector<int> &inputs = {});

  LogicGate(const std::string &data);

  virtual ~LogicGate();

  std::string toString() const;

  Type t() const;

  std::vector<std::vector<int>> cnf() const;

  static void reset();

  static inline int numInputs(Type typ) {
    switch (typ) {
      case Type::and_gate:
        return 2;
      case Type::or_gate:
        return 2;
      case Type::xor_gate:
        return 2;
      case Type::maj_gate:
        return 3;
      case Type::xor3_gate:
        return 3;
    }
  }

  static inline int numClausesCNF(Type typ) {
    switch (typ) {
      case Type::and_gate:
        return 3;
      case Type::or_gate:
        return 3;
      case Type::xor_gate:
        return 4;
      case Type::xor3_gate:
        return 8;
      case Type::maj_gate:
        return 6;
    }
  }

  int depth;
  int output;
  std::vector<int> inputs;
  static std::vector<LogicGate> global_gates;

 protected:
  Type t_;
};

}  // end namespace preimage
