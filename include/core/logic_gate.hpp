/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
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

  virtual ~LogicGate();

  std::string toString() const;

  static LogicGate fromString(const std::string &data);

  Type t() const;

  std::vector<std::vector<int>> cnf() const;

  static void reset();

  static inline std::string humanReadableType(Type typ) {
    switch (typ) {
      case Type::and_gate:
        return "AND";
      case Type::or_gate:
        return "OR";
      case Type::xor_gate:
        return "XOR-2";
      case Type::maj_gate:
        return "Maj-3";
      case Type::xor3_gate:
        return "XOR-3";
    }
  }

  int depth;
  int output;
  std::vector<int> inputs;
  thread_local static std::vector<LogicGate> global_gates;

 protected:
  Type t_;
};

}  // end namespace preimage
