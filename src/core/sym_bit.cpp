/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include "core/sym_bit.hpp"

#include <vector>

#include "core/config.hpp"
#include "core/logic_gate.hpp"

namespace preimage {

thread_local int SymBit::global_index = 1;  // 1-based indexing

SymBit::SymBit(bool bit_val, bool is_unknown) : val(bit_val), unknown(is_unknown) {
  if (unknown) {
    index = SymBit::global_index++;
  }
}

SymBit::~SymBit() {}

SymBit SymBit::zero() { return SymBit(0, false); }

SymBit SymBit::one() { return SymBit(1, false); }

SymBit SymBit::constant(bool val) { return SymBit(val, false); }

void SymBit::reset() { global_index = 1; }

SymBit SymBit::operator~() const {
  SymBit b(!val, false);
  b.index = -index;
  b.unknown = unknown;
  return b;
}

SymBit SymBit::operator&(const SymBit &b) const {
  // If AND-ing with a constant of 0, result will be always be 0
  const SymBit a = *this;
  if (!a.unknown && !a.val) return SymBit::zero();
  if (!b.unknown && !b.val) return SymBit::zero();

  if (a.unknown && b.unknown) {
    // (0 & 0 = 0), (1 & 1 = 1)
    if (a.index == b.index) return a;
    // (0 & 1 = 0), (1 & 0 = 0)
    if (a.index == -b.index) return SymBit::zero();

    SymBit result(a.val & b.val, true);
    LogicGate f(LogicGate::Type::and_gate, result.index, {a.index, b.index});
    LogicGate::global_gates.push_back(f);
    return result;
  } else if (a.unknown) {
    // Here, "b" is a constant equal to 1, so result is directly "a"
    return a;
  } else if (b.unknown) {
    // Here, "a" is a constant equal to 1, so result is directly "b"
    return b;
  } else {
    return SymBit::constant(a.val & b.val);  // Both are constants
  }
}

SymBit SymBit::operator^(const SymBit &b) const {
  const SymBit a = *this;

  if (a.unknown && b.unknown) {
    // a ^ a is always 0
    if (a.index == b.index) return SymBit::zero();
    // a ^ !a is always 1
    if (a.index == -b.index) return SymBit::one();

    if (config::only_and_gates) {
      const SymBit tmp1 = ~(a & b);
      const SymBit tmp2 = ~(a & tmp1);
      const SymBit tmp3 = ~(b & tmp1);
      return ~(tmp2 & tmp3);
    }

    SymBit result(a.val ^ b.val, true);
    LogicGate f(LogicGate::Type::xor_gate, result.index, {a.index, b.index});
    LogicGate::global_gates.push_back(f);
    return result;
  } else if (a.unknown) {
    // XOR with a constant of 0 is simply the other input
    if (!b.val) return a;
    // XOR with a constant of 1 is the inverse of the other input
    return ~a;
  } else if (b.unknown) {
    // XOR with a constant of 0 is simply the other input
    if (!a.val) return b;
    // XOR with a constant of 1 is the inverse of the other input
    return ~b;
  } else {
    return SymBit::constant(a.val ^ b.val);  // Both are constants
  }
}

SymBit SymBit::operator|(const SymBit &b) const {
  // If OR-ing with a constant of 1, result will be always be 1
  const SymBit a = *this;
  if (!a.unknown && a.val) return SymBit::one();
  if (!b.unknown && b.val) return SymBit::one();

  if (a.unknown && b.unknown) {
    // (0 | 0 = 0), (1 | 1 = 1)
    if (a.index == b.index) return a;
    // (0 | 1 = 1), (1 | 0 = 1)
    if (a.index == -b.index) return SymBit::one();

    if (config::only_and_gates) {
      const SymBit tmp1 = ~(a & a);
      const SymBit tmp2 = ~(b & b);
      return ~(tmp1 & tmp2);
    }

    SymBit result(a.val | b.val, true);
    LogicGate f(LogicGate::Type::or_gate, result.index, {a.index, b.index});
    LogicGate::global_gates.push_back(f);
    return result;
  } else if (a.unknown) {
    // Here, "b" is a constant equal to 0, so result is directly "a"
    return a;
  } else if (b.unknown) {
    // Here, "a" is a constant equal to 0, so result is directly "b"
    return b;
  } else {
    return SymBit::constant(a.val | b.val);  // Both are constants
  }
}

std::pair<SymBit, SymBit> SymBit::add(const SymBit &a, const SymBit &b) {
  return add(a, b, SymBit::zero());
}

std::pair<SymBit, SymBit> SymBit::add(const SymBit &a, const SymBit &b,
                                      const SymBit &carry_in) {
  const SymBit sum1 = xor3(a, b, carry_in);
  const SymBit carry_out = majority3(a, b, carry_in);
  return {sum1, carry_out};
}

SymBit SymBit::majority3(const SymBit &a, const SymBit &b, const SymBit &c) {
  const int sum = ((int)a.val) + ((int)b.val) + ((int)c.val);
  const bool val = (sum > 1);

  std::vector<bool> knowns;
  std::vector<SymBit> unknowns;
  if (!a.unknown)
    knowns.push_back(a.val);
  else
    unknowns.push_back(a);
  if (!b.unknown)
    knowns.push_back(b.val);
  else
    unknowns.push_back(b);
  if (!c.unknown)
    knowns.push_back(c.val);
  else
    unknowns.push_back(c);

  if (knowns.size() == 0) {
    // If any two inputs are the same, they have automatic majority
    if (a.index == b.index) return a;
    if (a.index == c.index) return a;
    if (b.index == c.index) return b;
    // If any two inputs are opposites, the result is the odd one out
    if (a.index == -b.index) return c;
    if (a.index == -c.index) return b;
    if (b.index == -c.index) return a;

    if (config::only_and_gates) {
      return (~(~a & ~b)) & (~(~a & ~c)) & (~(~b & ~c));
    }

    // All 3 inputs are unknown --> output will be unknown
    SymBit result(val, true);
    LogicGate f(LogicGate::Type::maj_gate, result.index, {a.index, b.index, c.index});
    LogicGate::global_gates.push_back(f);
    return result;
  } else if (knowns.size() == 1) {
    if (knowns.at(0) == false) {
      // 0 0 0 = 0  --> output = unknown1 & unknown2
      // 0 0 1 = 0
      // 0 1 0 = 0
      // 0 1 1 = 1
      return unknowns.at(0) & unknowns.at(1);
    } else {
      // 1 0 0 = 0 --> output = unknown1 | unknown2
      // 1 0 1 = 1
      // 1 1 0 = 1
      // 1 1 1 = 1
      return unknowns.at(0) | unknowns.at(1);
    }
  } else if (knowns.size() == 2) {
    // If known bits are equal (0, 0) or (1, 1) they will have the majority
    if (knowns.at(0) == knowns.at(1)) {
      return SymBit::constant(knowns.at(0));
    } else {
      return unknowns.at(0);  // Otherwise we have Maj3(0, 1, X) = X
    }
  } else {
    return SymBit::constant(val);  // Otherwise all 3 values are known, so not a RV
  }
}

SymBit SymBit::xor3(const SymBit &a, const SymBit &b, const SymBit &c) {
  const bool val = a.val ^ b.val ^ c.val;

  std::vector<bool> knowns;
  std::vector<SymBit> unknowns;
  if (!a.unknown)
    knowns.push_back(a.val);
  else
    unknowns.push_back(a);
  if (!b.unknown)
    knowns.push_back(b.val);
  else
    unknowns.push_back(b);
  if (!c.unknown)
    knowns.push_back(c.val);
  else
    unknowns.push_back(c);

  if (knowns.size() == 0) {
    // If any two inputs are the same, 0 ^ 0 ^ x = 0 ^ x = x
    if (a.index == b.index) return c;
    if (a.index == c.index) return b;
    if (b.index == c.index) return a;
    // If any two inputs are opposites, 0 ^ 1 ^ x = 1 ^ x = ~x
    if (a.index == -b.index) return ~c;
    if (a.index == -c.index) return ~b;
    if (b.index == -c.index) return ~a;

    if (config::only_and_gates) return a ^ b ^ c;

    // All 3 inputs are unknown --> output will be unknown
    SymBit result(val, true);
    LogicGate f(LogicGate::Type::xor3_gate, result.index, {a.index, b.index, c.index});
    LogicGate::global_gates.push_back(f);
    return result;
  } else if (knowns.size() == 1) {
    if (knowns.at(0) == false) {
      // 0 0 0 = 0  --> output = unknown1 ^ unknown2
      // 0 0 1 = 1
      // 0 1 0 = 1
      // 0 1 1 = 0
      return unknowns.at(0) ^ unknowns.at(1);
    } else {
      // 1 0 0 = 1 --> output = ~(unknown1 ^ unknown2)
      // 1 0 1 = 0
      // 1 1 0 = 0
      // 1 1 1 = 1
      return ~(unknowns.at(0) ^ unknowns.at(1));
    }
  } else if (knowns.size() == 2) {
    return unknowns.at(0) ^ SymBit::constant(knowns.at(0) ^ knowns.at(1));
  } else {
    return SymBit::constant(val);  // Otherwise all 3 values are known, so not a RV
  }
}

}  // end namespace preimage
