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

#include "core/logic_gate.hpp"

#include <iomanip>
#include <sstream>

namespace preimage {

std::vector<LogicGate> LogicGate::global_gates = {};

LogicGate::LogicGate() {}

LogicGate::LogicGate(LogicGate::Type typ, const int dpth, const int out,
                     const std::vector<int> &inp)
    : depth(dpth), output(out), inputs(inp), t_(typ) {
  const int n_inputs = numInputs(t_);
  if (n_inputs != inp.size()) {
    spdlog::info("Gate {} requires {} input(s) but got {}",
                 char(t_), n_inputs, inp.size());
  }
  assert(depth > 0);
  assert(output != 0);
  for (int i : inputs) assert(i != 0);
  assert(n_inputs == inp.size());
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
      return {{-output, inputs[0]},
              {-output, inputs[1]},
              {output, -inputs[0], -inputs[1]}};
    case Type::or_gate:
      return {{output, -inputs[0]},
              {output, -inputs[1]},
              {-output, inputs[0], inputs[1]}};
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
      return {{-output, inputs[0], inputs[1]},
              {-output, inputs[0], inputs[2]},
              {-output, inputs[1], inputs[2]},
              {output, -inputs[0], -inputs[1]},
              {output, -inputs[0], -inputs[2]},
              {output, -inputs[1], -inputs[2]}};
  }
}

}  // end namespace preimage