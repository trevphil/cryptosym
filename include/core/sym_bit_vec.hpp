/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#pragma once

#include <string>
#include <vector>

#include "core/bit_vec.hpp"
#include "core/sym_bit.hpp"

namespace preimage {

/*
Consider the number 0b1101 = 13.
As a SymBitVec, we get an array [1, 0, 1, 1] such that
the LSB is at index 0 and the MSB at index 3.
*/

class SymBitVec {
 public:
  SymBitVec();

  explicit SymBitVec(const std::vector<SymBit> &bits);

  SymBitVec(const BitVec &bits, bool unknown = false);

  SymBitVec(uint64_t n, unsigned int sz, bool unknown = false);

  virtual ~SymBitVec();

  unsigned int size() const;

  uint64_t intVal() const;

  BitVec bits() const;

  std::string bin() const;

  std::string hex() const;

  SymBit at(unsigned int index) const;

  SymBitVec concat(const SymBitVec &other) const;

  SymBitVec extract(unsigned int lb, unsigned int ub) const;

  SymBitVec resize(unsigned int n) const;

  SymBitVec rotr(unsigned int n) const;

  SymBitVec reversed() const;

  SymBitVec operator~() const;

  SymBitVec operator&(const SymBitVec &b) const;

  SymBitVec operator^(const SymBitVec &b) const;

  SymBitVec operator|(const SymBitVec &b) const;

  SymBitVec operator+(const SymBitVec &b) const;

  SymBitVec operator<<(unsigned int n) const;

  SymBitVec operator>>(unsigned int n) const;

  static SymBitVec majority3(const SymBitVec &a, const SymBitVec &b, const SymBitVec &c);

  static SymBitVec xor3(const SymBitVec &a, const SymBitVec &b, const SymBitVec &c);

 private:
  std::vector<SymBit> bits_;
};

}  // end namespace preimage
