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

#include "core/bit.hpp"

namespace preimage {

int Bit::global_index = 0;
std::vector<Bit> Bit::global_bits = {};

Bit::Bit(bool bit_val, bool rv, int d) : val(bit_val), is_rv(rv), depth(d) {
  if (is_rv) {
    index = Bit::global_index++;
    Bit::global_bits.push_back(*this);
  }
}

Bit::~Bit() {}

Bit Bit::zero() { return Bit(0, false, 0); }

Bit Bit::one() { return Bit(1, false, 0); }

Bit Bit::constant(bool val) { return Bit(val, false, 0); }

void Bit::reset() {
  global_index = 0;
  global_bits = {};
  Factor::reset();
}

Bit Bit::operator~() const {
  const Bit b(!val, is_rv, is_rv ? depth + 1 : 0);
  if (is_rv) {
    const Factor f(Factor::Type::NotFactor, b.index, {index});
    Factor::global_factors[b.index] = f;
  }
  return b;
}

Bit Bit::operator&(const Bit &b) const {
  // If AND-ing with a constant of 0, result will be always be 0
  if (!is_rv && !val) return Bit::zero();
  if (!b.is_rv && !b.val) return Bit::zero();

  const Bit a = *this;

  if (is_rv && b.is_rv) {
    // (0 & 0 = 0), (1 & 1 = 1)
    if (index == b.index) return a;
    Bit result(val & b.val, true, std::max(depth, b.depth) + 1);
    Factor f(Factor::Type::AndFactor, result.index, {index, b.index});
    Factor::global_factors[result.index] = f;
    return result;
  } else if (is_rv) {
    // Here, "b" is a constant equal to 1, so result is directly "a"
    return a;
  } else if (b.is_rv) {
    // Here, "a" is a constant equal to 1, so result is directly "b"
    return b;
  } else {
    return Bit::constant(val & b.val);  // Both are constants
  }
}

Bit Bit::operator^(const Bit &b) const {
  const Bit a = *this;

  if (is_rv && b.is_rv) {
    // a ^ a is always 0
    if (index == b.index) return Bit::zero();
    Bit result(val ^ b.val, true, std::max(depth, b.depth) + 1);
    Factor f(Factor::Type::XorFactor, result.index, {index, b.index});
    Factor::global_factors[result.index] = f;
    return result;
  } else if (a.is_rv) {
    // XOR with a constant of 0 is simply the other input
    if (!b.val) return a;
    // XOR with a constant of 1 is the inverse of the other input
    return ~a;
  } else if (b.is_rv) {
    // XOR with a constant of 0 is simply the other input
    if (!a.val) return b;
    // XOR with a constant of 1 is the inverse of the other input
    return ~b;
  } else {
    return Bit::constant(val ^ b.val);  // Both are constants
  }
}

Bit Bit::operator|(const Bit &b) const {
  // If OR-ing with a constant of 1, result will be always be 1
  if (!is_rv && val) return Bit::one();
  if (!b.is_rv && b.val) return Bit::one();

  const Bit a = *this;

  if (is_rv && b.is_rv) {
    // (0 | 0 = 0), (1 | 1 = 1)
    if (index == b.index) return a;
    Bit result(val | b.val, true, std::max(depth, b.depth) + 1);
    Factor f(Factor::Type::OrFactor, result.index, {index, b.index});
    Factor::global_factors[result.index] = f;
    return result;
  } else if (is_rv) {
    // Here, "b" is a constant equal to 0, so result is directly "a"
    return a;
  } else if (b.is_rv) {
    // Here, "a" is a constant equal to 0, so result is directly "b"
    return b;
  } else {
    return Bit::constant(val | b.val);  // Both are constants
  }
}

std::pair<Bit, Bit> Bit::add(const Bit &a, const Bit &b) {
  return add(a, b, Bit::zero());
}

std::pair<Bit, Bit> Bit::add(const Bit &a, const Bit &b, const Bit &carry_in) {
  const Bit sum1 = a ^ (b ^ carry_in);
  const Bit carry_out = majority3(a, b, carry_in);
  return {sum1, carry_out};
}

Bit Bit::majority3(const Bit &a, const Bit &b, const Bit &c) {
  const int sum = ((int)a.val) + ((int)b.val) + ((int)c.val);
  const bool val = (sum > 1);

  std::vector<bool> knowns;
  std::vector<Bit> unknowns;
  if (!a.is_rv) knowns.push_back(a.val);
  else unknowns.push_back(a);
  if (!b.is_rv) knowns.push_back(b.val);
  else unknowns.push_back(b);
  if (!c.is_rv) knowns.push_back(c.val);
  else unknowns.push_back(c);

  if (knowns.size() == 0) {
    // If any two inputs are the same, they have automatic majority
    if (a.index == b.index) return a;
    if (a.index == c.index) return a;
    if (b.index == c.index) return b;
    // All 3 inputs are unknown and different --> output will be unknown
    Bit result(val, true, std::max(a.depth, std::max(b.depth, c.depth)) + 1);
    Factor f(Factor::Type::MajFactor, result.index, {a.index, b.index, c.index});
    Factor::global_factors[result.index] = f;
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

}  // end namespace preimage
