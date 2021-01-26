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

#include "bit.hpp"

#include "config.hpp"

namespace dataset_generator {

size_t Bit::global_index = 0;
std::vector<Bit> Bit::global_bits = {};

Bit::Bit(bool bit_val, bool rv, bool is_prior) : val(bit_val), is_rv(rv) {
  if (!is_rv) return;

  index = Bit::global_index++;
  Bit::global_bits.push_back(*this);
  if (is_prior) {
    const Factor f(Factor::Type::PriorFactor, index);
    Factor::global_factors.push_back(f);
  }
}

Bit::~Bit() {}

void Bit::reset() {
  global_index = 0;
  global_bits = {};
  Factor::reset();
}

Bit Bit::operator~() const {
  const Bit b(!val, is_rv);
  if (is_rv) {
    const Factor f(Factor::Type::NotFactor, b.index, {index});
    Factor::global_factors.push_back(f);
  }
  return b;
}

Bit Bit::operator&(const Bit &b) const {
  // If AND-ing with a constant of 0, result will be always be 0
  if (!is_rv && !val) return Bit(0, false);
  if (!b.is_rv && !b.val) return Bit(0, false);

  const Bit a = *this;

  if (is_rv && b.is_rv) {
    // (0 & 0 = 0), (1 & 1 = 1)
    if (index == b.index) return a;
    Bit result(val & b.val, true);
    Factor f(Factor::Type::AndFactor, result.index, {index, b.index});
    Factor::global_factors.push_back(f);
    return result;
  } else if (is_rv) {
    // Here, "b" is a constant equal to 1, so result is directly "a"
    return a;
  } else if (b.is_rv) {
    // Here, "a" is a constant equal to 1, so result is directly "b"
    return b;
  } else {
    return Bit(val & b.val, false);  // Both are constants
  }
}

Bit Bit::operator^(const Bit &b) const {
  const Bit a = *this;

  if (is_rv && b.is_rv) {
    // a ^ a is always 0
    if (index == b.index) return Bit(0, false);
    if (config::use_xor) {
      Bit result(val ^ b.val, true);
      Factor f(Factor::Type::XorFactor, result.index, {index, b.index});
      Factor::global_factors.push_back(f);
      return result;
    } else {
      Bit tmp1 = ~(a & b);
      Bit tmp2 = ~(a & tmp1);
      Bit tmp3 = ~(b & tmp1);
      return ~(tmp2 & tmp3);
    }
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
    return Bit(val ^ b.val, false);  // Both are constants
  }
}

Bit Bit::operator|(const Bit &b) const {
  // If OR-ing with a constant of 1, result will be always be 1
  if (!is_rv && val) return Bit(1, false);
  if (!b.is_rv && b.val) return Bit(1, false);

  const Bit a = *this;

  if (is_rv && b.is_rv) {
    // (0 | 0 = 0), (1 | 1 = 1)
    if (index == b.index) return a;
    Bit tmp1 = ~(a & a);
    Bit tmp2 = ~(b & b);
    return ~(tmp1 & tmp2);
  } else if (is_rv) {
    // Here, "b" is a constant equal to 0, so result is directly "a"
    return a;
  } else if (b.is_rv) {
    // Here, "a" is a constant equal to 0, so result is directly "b"
    return b;
  } else {
    return Bit(val | b.val, false);  // Both are constants
  }
}

std::pair<Bit, Bit> Bit::add(const Bit &a, const Bit &b) {
  return add(a, b, Bit(0, false));
}

std::pair<Bit, Bit> Bit::add(const Bit &a, const Bit &b, const Bit &carry_in) {
  const Bit sum1 = a ^ b;
  const Bit carry1 = a & b;
  const Bit sum2 = carry_in ^ sum1;
  const Bit carry2 = carry_in & sum1;
  const Bit carry_out = carry1 | carry2;
  return {sum2, carry_out};
}

}  // end namespace dataset_generator
