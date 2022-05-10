/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
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
  SymBitVec(uint64_t n, unsigned int sz, bool unknown = false);

  virtual ~SymBitVec();

  unsigned int size() const;

  uint64_t intVal() const;

  boost::dynamic_bitset<> bits() const;

  std::string bin() const;

  std::string hex() const;

  Bit at(unsigned int index) const;

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
  std::vector<Bit> bits_;
};

}  // end namespace preimage
