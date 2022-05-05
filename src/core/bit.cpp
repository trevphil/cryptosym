/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include "core/bit.hpp"

#include <vector>

#include "core/config.hpp"
#include "core/logic_gate.hpp"

namespace preimage {

thread_local int Bit::global_index = 1;  // 1-based indexing

Bit::Bit(bool bit_val, bool is_unknown, int dpth)
    : val(bit_val), unknown(is_unknown), depth(dpth) {
  if (unknown) {
    index = Bit::global_index++;
  }
}

Bit::~Bit() {}

Bit Bit::zero() { return Bit(0, false, 0); }

Bit Bit::one() { return Bit(1, false, 0); }

Bit Bit::constant(bool val) { return Bit(val, false, 0); }

void Bit::reset() { global_index = 1; }

Bit Bit::operator~() const {
  Bit b(!val, false, 0);
  b.index = -index;
  b.unknown = unknown;
  b.depth = depth;
  return b;
}

Bit Bit::operator&(const Bit &b) const {
  // If AND-ing with a constant of 0, result will be always be 0
  const Bit a = *this;
  if (!a.unknown && !a.val) return Bit::zero();
  if (!b.unknown && !b.val) return Bit::zero();

  if (a.unknown && b.unknown) {
    // (0 & 0 = 0), (1 & 1 = 1)
    if (a.index == b.index) return a;
    // (0 & 1 = 0), (1 & 0 = 0)
    if (a.index == -b.index) return Bit::zero();

    Bit result(a.val & b.val, true, 1 + std::max(a.depth, b.depth));
    LogicGate f(LogicGate::Type::and_gate, result.depth, result.index,
                {a.index, b.index});
    LogicGate::global_gates.push_back(f);
    return result;
  } else if (a.unknown) {
    // Here, "b" is a constant equal to 1, so result is directly "a"
    return a;
  } else if (b.unknown) {
    // Here, "a" is a constant equal to 1, so result is directly "b"
    return b;
  } else {
    return Bit::constant(a.val & b.val);  // Both are constants
  }
}

Bit Bit::operator^(const Bit &b) const {
  const Bit a = *this;

  if (a.unknown && b.unknown) {
    // a ^ a is always 0
    if (a.index == b.index) return Bit::zero();
    // a ^ !a is always 1
    if (a.index == -b.index) return Bit::one();

    if (config::only_and_gates) {
      const Bit tmp1 = ~(a & b);
      const Bit tmp2 = ~(a & tmp1);
      const Bit tmp3 = ~(b & tmp1);
      return ~(tmp2 & tmp3);
    }

    Bit result(a.val ^ b.val, true, 1 + std::max(a.depth, b.depth));
    LogicGate f(LogicGate::Type::xor_gate, result.depth, result.index,
                {a.index, b.index});
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
    return Bit::constant(a.val ^ b.val);  // Both are constants
  }
}

Bit Bit::operator|(const Bit &b) const {
  // If OR-ing with a constant of 1, result will be always be 1
  const Bit a = *this;
  if (!a.unknown && a.val) return Bit::one();
  if (!b.unknown && b.val) return Bit::one();

  if (a.unknown && b.unknown) {
    // (0 | 0 = 0), (1 | 1 = 1)
    if (a.index == b.index) return a;
    // (0 | 1 = 1), (1 | 0 = 1)
    if (a.index == -b.index) return Bit::one();

    if (config::only_and_gates) {
      const Bit tmp1 = ~(a & a);
      const Bit tmp2 = ~(b & b);
      return ~(tmp1 & tmp2);
    }

    Bit result(a.val | b.val, true, 1 + std::max(a.depth, b.depth));
    LogicGate f(LogicGate::Type::or_gate, result.depth, result.index, {a.index, b.index});
    LogicGate::global_gates.push_back(f);
    return result;
  } else if (a.unknown) {
    // Here, "b" is a constant equal to 0, so result is directly "a"
    return a;
  } else if (b.unknown) {
    // Here, "a" is a constant equal to 0, so result is directly "b"
    return b;
  } else {
    return Bit::constant(a.val | b.val);  // Both are constants
  }
}

std::pair<Bit, Bit> Bit::add(const Bit &a, const Bit &b) {
  return add(a, b, Bit::zero());
}

std::pair<Bit, Bit> Bit::add(const Bit &a, const Bit &b, const Bit &carry_in) {
  const Bit sum1 = xor3(a, b, carry_in);
  const Bit carry_out = majority3(a, b, carry_in);
  return {sum1, carry_out};
}

Bit Bit::majority3(const Bit &a, const Bit &b, const Bit &c) {
  const int sum = ((int)a.val) + ((int)b.val) + ((int)c.val);
  const bool val = (sum > 1);

  std::vector<bool> knowns;
  std::vector<Bit> unknowns;
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
    Bit result(val, true, 1 + std::max(a.depth, std::max(b.depth, c.depth)));
    LogicGate f(LogicGate::Type::maj_gate, result.depth, result.index,
                {a.index, b.index, c.index});
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
      return Bit::constant(knowns.at(0));
    } else {
      return unknowns.at(0);  // Otherwise we have Maj3(0, 1, X) = X
    }
  } else {
    return Bit::constant(val);  // Otherwise all 3 values are known, so not a RV
  }
}

Bit Bit::xor3(const Bit &a, const Bit &b, const Bit &c) {
  const bool val = a.val ^ b.val ^ c.val;

  std::vector<bool> knowns;
  std::vector<Bit> unknowns;
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
    Bit result(val, true, 1 + std::max(a.depth, std::max(b.depth, c.depth)));
    LogicGate f(LogicGate::Type::xor3_gate, result.depth, result.index,
                {a.index, b.index, c.index});
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
    return unknowns.at(0) ^ Bit::constant(knowns.at(0) ^ knowns.at(1));
  } else {
    return Bit::constant(val);  // Otherwise all 3 values are known, so not a RV
  }
}

}  // end namespace preimage
