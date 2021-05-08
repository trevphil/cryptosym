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

#include "core/sym_bit_vec.hpp"
#include "core/utils.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>
#include <ostream>

namespace preimage {

SymBitVec::SymBitVec() : bits_({}) {}

SymBitVec::SymBitVec(const std::vector<Bit> &bits) : bits_(bits) {}

SymBitVec::SymBitVec(const boost::dynamic_bitset<> &bits, bool unknown) {
  bits_ = {};
  bits_.reserve(bits.size());
  const bool is_prior = unknown;
  for (boost::dynamic_bitset<>::size_type i = 0; i < bits.size(); ++i) {
    bits_.push_back(Bit(bits[i], unknown, is_prior));
  }
}

SymBitVec::SymBitVec(uint64_t n, size_t sz, bool unknown) {
  if (sz > sizeof(uint64_t) * 8) {
    spdlog::warn("Initializing SymBitVec[{}] with {}-bit integer", sz,
                 sizeof(uint64_t) * 8);
  }
  bits_ = {};
  bits_.reserve(sz);
  const bool is_prior = unknown;
  for (size_t i = 0; i < sz; i++) {
    Bit b((n >> i) & 1, unknown, is_prior);
    bits_.push_back(b);
  }
}

SymBitVec::~SymBitVec() {}

size_t SymBitVec::size() const { return bits_.size(); }

uint64_t SymBitVec::intVal() const {
  const size_t n = size();
  if (n > sizeof(uint64_t) * 8) {
    spdlog::warn("Integer overflow, coercing {}-bit SymBitVec to {}-bit int", n,
                 sizeof(uint64_t) * 8);
  }
  uint64_t result = 0;
  for (size_t i = 0; i < n; ++i) {
    uint64_t val = at(i).val & 1;
    result += (val << i);
  }
  return result;
}

boost::dynamic_bitset<> SymBitVec::bits() const {
  const size_t n = size();
  boost::dynamic_bitset<> b(n);
  for (size_t i = 0; i < n; ++i) b[i] = at(i).val;
  return b;
}

std::string SymBitVec::bin(bool colored) const {
  if (!colored) return Utils::binstr(bits());
  std::stringstream bit_stream;
  const size_t n = size();
  for (size_t i = 0; i < n; ++i) {
    if (at(i).is_rv) {
      bit_stream << "\033[32m" << (at(i).val ? '1' : '0') << "\033[0m";
    } else {
      bit_stream << (at(i).val ? '1' : '0');
    }
  }
  return bit_stream.str();
}

std::string SymBitVec::hex() const { return Utils::hexstr(bits()); }

Bit SymBitVec::at(size_t index) const {
  const size_t n = size();
  if (index >= n) spdlog::error("Index {} o.o.b. for SymBitVec[{}]", index, n);
  assert(index < n);
  return bits_.at(index);
}

std::vector<size_t> SymBitVec::rvIndices() const {
  std::vector<size_t> indices;
  for (const Bit &b : bits_) {
    if (b.is_rv) indices.push_back(b.index);
  }
  return indices;
}

SymBitVec SymBitVec::concat(const SymBitVec &other) const {
  /*
   Consider A = 0b1101 = 13 = [1, 0, 1, 1]
            B = 0b1000 = 8  = [0, 0, 0, 1]
   Then A.concat(B) gives [1, 0, 1, 1, 0, 0, 0, 1] = 141
                          LSB                  MSB
  */
  std::vector<Bit> bits = bits_;
  bits.insert(bits.end(), other.bits_.begin(), other.bits_.end());
  return SymBitVec(bits);
}

SymBitVec SymBitVec::extract(size_t lb, size_t ub) const {
  /*
   Consider A = 0b1101 = 13 = [1, 0, 1, 1]
   Then A.extract(1, 3) gives [0, 1] = 0b10 = 2 (upper bound is exclusive)
  */
  std::vector<Bit> bits;
  for (size_t i = lb; i < ub; i++) bits.push_back(at(i));
  return SymBitVec(bits);
}

SymBitVec SymBitVec::resize(size_t n) const {
  const size_t m = size();
  if (n == m) return *this;

  std::vector<Bit> bits;
  if (n > 0 && n < m) {
    for (int i = 0; i < n; i++) bits.push_back(at(i));
  } else if (n > m) {
    const size_t num_zeros = n - m;
    bits = bits_;
    for (int i = 0; i < num_zeros; i++) bits.push_back(Bit(0, false));
  }
  return SymBitVec(bits);
}

SymBitVec SymBitVec::rotr(size_t n) const {
  const size_t sz = size();
  std::vector<Bit> bits = bits_;
  std::rotate(bits.begin(), bits.begin() + sz - n, bits.end());
  return SymBitVec(bits);
}

SymBitVec SymBitVec::reversed() const {
  std::vector<Bit> bits = bits_;
  std::reverse(bits.begin(), bits.end());
  return SymBitVec(bits);
}

SymBitVec SymBitVec::operator~() const {
  std::vector<Bit> bits = {};
  for (const Bit &b : bits_) bits.push_back(~b);
  return SymBitVec(bits);
}

SymBitVec SymBitVec::operator&(const SymBitVec &b) const {
  const size_t n = size();
  assert(n == b.size());
  std::vector<Bit> bits = {};
  for (int i = 0; i < n; i++) bits.push_back(at(i) & b.at(i));
  return SymBitVec(bits);
}

SymBitVec SymBitVec::operator^(const SymBitVec &b) const {
  const size_t n = size();
  assert(n == b.size());
  std::vector<Bit> bits = {};
  for (int i = 0; i < n; i++) bits.push_back(at(i) ^ b.at(i));
  return SymBitVec(bits);
}

SymBitVec SymBitVec::operator|(const SymBitVec &b) const {
  const size_t n = size();
  assert(n == b.size());
  std::vector<Bit> bits = {};
  for (int i = 0; i < n; i++) bits.push_back(at(i) | b.at(i));
  return SymBitVec(bits);
}

SymBitVec SymBitVec::operator+(const SymBitVec &b) const {
  const size_t m = size();
  const size_t n = b.size();
  assert(m == n);

  Bit carry(0, false);
  std::vector<Bit> output_bits = {};
  output_bits.reserve(n);

  for (size_t i = 0; i < n; ++i) {
    const auto result = Bit::add(at(i), b.at(i), carry);
    output_bits.push_back(result.first);
    carry = result.second;
  }

  return SymBitVec(output_bits);
}

SymBitVec SymBitVec::operator<<(size_t n) const {
  /*
   Left-shift increases the value, i.e. 0b1101 << 1 = 0b11010.
   This adds "n" zeros in the LSB section (beginning of array).
   The first (m - n) bits of the original array are not chopped off.
  */
  if (n == 0) return *this;
  const size_t m = size();
  std::vector<Bit> bits = {};
  for (size_t i = 0; i < n; i++) bits.push_back(Bit(0, false));
  for (size_t i = 0; i < m - n; i++) bits.push_back(at(i));
  SymBitVec result(bits);
  assert(result.size() == m);
  return result;
}

SymBitVec SymBitVec::operator>>(size_t n) const {
  /*
   Right-shift decreases the value, i.e. 0b1101 >> 1 = 0b110.
     [1, 0, 1, 1] --> [0, 1, 1] + [0]
   This adds "n" zeros in the MSB section (end of array).
   The first "n" bits of the original array are chopped off
  */
  if (n == 0) return *this;
  const size_t m = size();
  std::vector<Bit> bits = {};
  for (size_t i = n; i < m; i++) bits.push_back(at(i));
  for (size_t i = 0; i < n; i++) bits.push_back(Bit(0, false));
  SymBitVec result(bits);
  assert(result.size() == m);
  return result;
}

}  // end namespace preimage
