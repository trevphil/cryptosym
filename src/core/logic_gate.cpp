/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
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

  char err_msg[256];
  snprintf(err_msg, 256, "Unsupported logic gate: %c", (char)t);
  throw std::invalid_argument(err_msg);
}

LogicGate::LogicGate() {}

LogicGate::LogicGate(LogicGate::Type typ, const int out, const std::vector<int> &inp)
    : output(out), inputs(inp), t_(typ) {
  const int n_inputs = numInputs(t_);
  if (n_inputs != static_cast<int>(inp.size())) {
    char err_msg[128];
    snprintf(err_msg, 128, "%s gate requires %d input(s) but got %lu\n",
             LogicGate::humanReadableType(t_).c_str(), n_inputs, inp.size());
    throw std::invalid_argument(err_msg);
  }
  if (output <= 0) {
    char err_msg[128];
    snprintf(err_msg, 128, "Logic gate output index must be > 0 (got %d)", output);
    throw std::invalid_argument(err_msg);
  }
  for (int i : inputs) {
    if (i == 0) {
      throw std::invalid_argument("Logic gate input index cannot be 0");
    }
  }
}

LogicGate::~LogicGate() {}

LogicGate::Type LogicGate::t() const { return t_; }

std::string LogicGate::toString() const {
  std::stringstream stream;
  stream << char(t_) << " " << output;
  for (int inp : inputs) stream << " " << inp;
  std::string result(stream.str());
  return result;
}

LogicGate LogicGate::fromString(const std::string &data) {
  const Type t = static_cast<Type>(data[0]);
  int output;
  std::istringstream nums(data.substr(1));
  nums >> output;
  const int n_inputs = numInputs(t);
  std::vector<int> inputs(n_inputs);
  for (int i = 0; i < n_inputs; i++) nums >> inputs[i];
  return LogicGate(t, output, inputs);
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

  char err_msg[256];
  snprintf(err_msg, 256, "CNF not implemented for logic gate: %c", (char)t_);
  throw std::runtime_error(err_msg);
}

}  // end namespace preimage
