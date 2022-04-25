/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include "core/logic_gate.hpp"

#include <iomanip>
#include <sstream>

namespace preimage {

thread_local std::vector<LogicGate> LogicGate::global_gates = {};

int numInputs(LogicGate::Type t) {
  switch (t) {
    case LogicGate::Type::and_gate:
      return 2;
    case LogicGate::Type::or_gate:
      return 2;
    case LogicGate::Type::xor_gate:
      return 2;
    case LogicGate::Type::maj_gate:
      return 3;
    case LogicGate::Type::xor3_gate:
      return 3;
  }
}

LogicGate::LogicGate() {}

LogicGate::LogicGate(LogicGate::Type typ, const int dpth, const int out,
                     const std::vector<int> &inp)
    : depth(dpth), output(out), inputs(inp), t_(typ) {
  const int n_inputs = numInputs(t_);
  if (n_inputs != inp.size()) {
    printf("Gate %c requires %d input(s) but got %lu\n", char(t_), n_inputs, inp.size());
  }
  assert(depth > 0);
  assert(output != 0);
  for (int i : inputs) assert(i != 0);
}

LogicGate::LogicGate(const std::string &data) {
  t_ = static_cast<Type>(data[0]);
  std::istringstream nums(data.substr(1));
  nums >> depth;
  assert(depth > 0);
  nums >> output;
  assert(output != 0);
  const int n_inputs = numInputs(t_);
  inputs = std::vector<int>(n_inputs);
  for (int i = 0; i < n_inputs; i++) {
    nums >> inputs[i];
    assert(inputs[i] != 0);
  }
}

LogicGate::~LogicGate() {}

LogicGate::Type LogicGate::t() const { return t_; }

std::string LogicGate::toString() const {
  std::stringstream stream;
  stream << char(t_) << " " << depth << " " << output;
  for (int inp : inputs) stream << " " << inp;
  std::string result(stream.str());
  return result;
}

void LogicGate::reset() { global_gates.clear(); }

std::vector<std::vector<int>> LogicGate::cnf() const {
  switch (t_) {
    case Type::and_gate:
      return {
          {-output, inputs[0]}, {-output, inputs[1]}, {output, -inputs[0], -inputs[1]}};
    case Type::or_gate:
      return {
          {output, -inputs[0]}, {output, -inputs[1]}, {-output, inputs[0], inputs[1]}};
    case Type::xor_gate:
      return {{output, inputs[0], -inputs[1]},
              {output, -inputs[0], inputs[1]},
              {-output, inputs[0], inputs[1]},
              {-output, -inputs[0], -inputs[1]}};
    case Type::xor3_gate:
      return {{output, inputs[0], inputs[1], -inputs[2]},
              {output, inputs[0], -inputs[1], inputs[2]},
              {output, -inputs[0], inputs[1], inputs[2]},
              {output, -inputs[0], -inputs[1], -inputs[2]},
              {-output, inputs[0], inputs[1], inputs[2]},
              {-output, inputs[0], -inputs[1], -inputs[2]},
              {-output, -inputs[0], inputs[1], -inputs[2]},
              {-output, -inputs[0], -inputs[1], inputs[2]}};
    case Type::maj_gate:
      return {{-output, inputs[0], inputs[1]},  {-output, inputs[0], inputs[2]},
              {-output, inputs[1], inputs[2]},  {output, -inputs[0], -inputs[1]},
              {output, -inputs[0], -inputs[2]}, {output, -inputs[1], -inputs[2]}};
  }
}

}  // end namespace preimage