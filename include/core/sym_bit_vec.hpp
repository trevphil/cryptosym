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

#include <boost/dynamic_bitset.hpp>
#include <string>
#include <vector>

#include "core/bit.hpp"

namespace preimage {

/*
Consider the number 0b1101 = 13.
As a SymBitVec, we get an array [1, 0, 1, 1] such that
the LSB is at index 0 and the MSB at index 3.
*/

class SymBitVec {
 public:
  SymBitVec();
  explicit SymBitVec(const std::vector<Bit> &bits);
  SymBitVec(const boost::dynamic_bitset<> &bits, bool unknown = false);
  SymBitVec(uint64_t n, int sz, bool unknown = false);

  virtual ~SymBitVec();

  int size() const;

  uint64_t intVal() const;

  boost::dynamic_bitset<> bits() const;

  std::string bin(bool colored = true) const;

  std::string hex() const;

  Bit at(int index) const;

  SymBitVec concat(const SymBitVec &other) const;

  SymBitVec extract(int lb, int ub) const;

  SymBitVec resize(int n) const;

  SymBitVec rotr(int n) const;

  SymBitVec reversed() const;

  SymBitVec operator~() const;

  SymBitVec operator&(const SymBitVec &b) const;

  SymBitVec operator^(const SymBitVec &b) const;

  SymBitVec operator|(const SymBitVec &b) const;

  SymBitVec operator+(const SymBitVec &b) const;

  SymBitVec operator<<(int n) const;

  SymBitVec operator>>(int n) const;

  static SymBitVec majority3(const SymBitVec &a, const SymBitVec &b, const SymBitVec &c);

  static SymBitVec xor3(const SymBitVec &a, const SymBitVec &b, const SymBitVec &c);

 private:
  std::vector<Bit> bits_;
};

}  // end namespace preimage
