/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include "core/sym_bit_vec.hpp"

#include <algorithm>
#include <stdexcept>

#include "core/utils.hpp"

namespace preimage {

SymBitVec::SymBitVec() : bits_({}) {}

SymBitVec::SymBitVec(const std::vector<SymBit> &bits) : bits_(bits) {}

SymBitVec::SymBitVec(const BitVec &bits, bool unknown) {
  bits_ = {};
  bits_.reserve(bits.size());
  for (unsigned int i = 0; i < bits.size(); ++i) {
    bits_.push_back(SymBit(bits[i], unknown));
  }
}

SymBitVec::SymBitVec(uint64_t n, unsigned int sz, bool unknown) {
  bits_ = {};
  bits_.reserve(sz);
  for (unsigned int i = 0; i < sz; i++) {
    bits_.push_back(SymBit((n >> i) & 1, unknown));
  }
}

SymBitVec::~SymBitVec() {}

unsigned int SymBitVec::size() const { return bits_.size(); }

uint64_t SymBitVec::intVal() const {
  const unsigned int n = size();
  if (n > sizeof(uint64_t) * 8) {
    const unsigned int max_bits = static_cast<unsigned int>(sizeof(uint64_t) * 8);
    printf("Possible int overflow, coercing %u-bit SymBitVec to %u-bit int\n", n,
           max_bits);
  }
  uint64_t result = 0;
  for (unsigned int i = 0; i < n; ++i) {
    uint64_t val = at(i).val & 1;
    result += (val << i);
  }
  return result;
}

BitVec SymBitVec::bits() const {
  const unsigned int n = size();
  BitVec b(n);
  for (unsigned int i = 0; i < n; ++i) b[i] = at(i).val;
  return b;
}

std::string SymBitVec::bin() const { return utils::binstr(bits()); }

std::string SymBitVec::hex() const { return utils::hexstr(bits()); }

SymBit SymBitVec::at(unsigned int index) const {
  const unsigned int n = size();
  if (index >= n) {
    char err_msg[128];
    snprintf(err_msg, 128, "Index %u out of bounds for SymBitVec[%u]", index, n);
    throw std::out_of_range(err_msg);
  }
  return bits_.at(index);
}

SymBitVec SymBitVec::concat(const SymBitVec &other) const {
  /*
   Consider A = 0b1101 = 13 = [1, 0, 1, 1]
            B = 0b1000 = 8  = [0, 0, 0, 1]
   Then A.concat(B) gives [1, 0, 1, 1, 0, 0, 0, 1] = 141
                          LSB                  MSB
  */
  std::vector<SymBit> bits = bits_;
  bits.insert(bits.end(), other.bits_.begin(), other.bits_.end());
  return SymBitVec(bits);
}

SymBitVec SymBitVec::extract(unsigned int lb, unsigned int ub) const {
  /*
   Consider A = 0b1101 = 13 = [1, 0, 1, 1]
   Then A.extract(1, 3) gives [0, 1] = 0b10 = 2 (upper bound is exclusive)
  */
  if (ub < lb) {
    char err_msg[128];
    snprintf(err_msg, 128, "%s, got [%u, %u]",
             "Lower bound of `extract` must be less than upper bound", lb, ub);
    throw std::out_of_range(err_msg);
  }

  std::vector<SymBit> bits;
  for (unsigned int i = lb; i < ub; i++) {
    bits.push_back(at(i));
  }
  return SymBitVec(bits);
}

SymBitVec SymBitVec::resize(unsigned int n) const {
  const unsigned int m = size();
  if (n == m) return *this;

  std::vector<SymBit> bits;
  if (n > 0 && n < m) {
    for (unsigned int i = 0; i < n; i++) bits.push_back(at(i));
  } else if (n > m) {
    const unsigned int num_zeros = n - m;
    bits = bits_;
    for (unsigned int i = 0; i < num_zeros; i++) {
      bits.push_back(SymBit::zero());
    }
  }
  return SymBitVec(bits);
}

SymBitVec SymBitVec::rotr(unsigned int n) const {
  n = n % size();
  std::vector<SymBit> bits = bits_;
  std::rotate(bits.begin(), bits.begin() + n, bits.end());
  return SymBitVec(bits);
}

SymBitVec SymBitVec::rotl(unsigned int n) const {
  const unsigned int sz = size();
  n = n % sz;
  std::vector<SymBit> bits = bits_;
  std::rotate(bits.begin(), bits.begin() + sz - n, bits.end());
  return SymBitVec(bits);
}

SymBitVec SymBitVec::reversed() const {
  std::vector<SymBit> bits = bits_;
  std::reverse(bits.begin(), bits.end());
  return SymBitVec(bits);
}

SymBitVec SymBitVec::operator~() const {
  std::vector<SymBit> bits = {};
  for (const SymBit &b : bits_) bits.push_back(~b);
  return SymBitVec(bits);
}

SymBitVec SymBitVec::operator&(const SymBitVec &b) const {
  const unsigned int n = size();
  if (n != b.size()) {
    char err_msg[128];
    snprintf(err_msg, 128, "Bit vectors must be same size (%u != %u)", n, b.size());
    throw std::length_error(err_msg);
  }
  std::vector<SymBit> bits = {};
  for (unsigned int i = 0; i < n; i++) bits.push_back(at(i) & b.at(i));
  return SymBitVec(bits);
}

SymBitVec SymBitVec::operator^(const SymBitVec &b) const {
  const unsigned int n = size();
  if (n != b.size()) {
    char err_msg[128];
    snprintf(err_msg, 128, "Bit vectors must be same size (%u != %u)", n, b.size());
    throw std::length_error(err_msg);
  }
  std::vector<SymBit> bits = {};
  for (unsigned int i = 0; i < n; i++) bits.push_back(at(i) ^ b.at(i));
  return SymBitVec(bits);
}

SymBitVec SymBitVec::operator|(const SymBitVec &b) const {
  const unsigned int n = size();
  if (n != b.size()) {
    char err_msg[128];
    snprintf(err_msg, 128, "Bit vectors must be same size (%u != %u)", n, b.size());
    throw std::length_error(err_msg);
  }
  std::vector<SymBit> bits = {};
  for (unsigned int i = 0; i < n; i++) bits.push_back(at(i) | b.at(i));
  return SymBitVec(bits);
}

SymBitVec SymBitVec::operator+(const SymBitVec &b) const {
  const unsigned int m = size();
  const unsigned int n = b.size();
  if (m != n) {
    char err_msg[128];
    snprintf(err_msg, 128, "Bit vectors must be same size (%u != %u)", m, n);
    throw std::length_error(err_msg);
  }

  SymBit carry = SymBit::zero();
  std::vector<SymBit> output_bits = {};
  output_bits.reserve(n);

  for (unsigned int i = 0; i < n; ++i) {
    const auto result = SymBit::add(at(i), b.at(i), carry);
    output_bits.push_back(result.first);
    carry = result.second;
  }

  return SymBitVec(output_bits);
}

SymBitVec SymBitVec::operator<<(unsigned int n) const {
  /*
   Left-shift increases the value, i.e. 0b1101 << 1 = 0b11010.
   This adds "n" zeros in the LSB section (beginning of array).
   The first (m - n) bits of the original array are not chopped off.
  */
  if (n == 0) return *this;
  const unsigned int m = size();
  n = std::min(m, n);
  std::vector<SymBit> bits = {};
  for (unsigned int i = 0; i < n; i++) bits.push_back(SymBit::zero());
  for (unsigned int i = 0; i < m - n; i++) bits.push_back(at(i));
  SymBitVec result(bits);
  return result;
}

SymBitVec SymBitVec::operator>>(unsigned int n) const {
  /*
   Right-shift decreases the value, i.e. 0b1101 >> 1 = 0b110.
     [1, 0, 1, 1] --> [0, 1, 1] + [0]
   This adds "n" zeros in the MSB section (end of array).
   The first "n" bits of the original array are chopped off
  */
  if (n == 0) return *this;
  const unsigned int m = size();
  n = std::min(m, n);
  std::vector<SymBit> bits = {};
  for (unsigned int i = n; i < m; i++) bits.push_back(at(i));
  for (unsigned int i = 0; i < n; i++) bits.push_back(SymBit::zero());
  SymBitVec result(bits);
  return result;
}


bool SymBitVec::operator==(const SymBitVec &b) const {
  if (size() != b.size()) return false;
  for (unsigned int i = 0; i < size(); ++i) {
    if (at(i).val != b.at(i).val) return false;
  }
  return true;
}

bool SymBitVec::operator!=(const SymBitVec &b) const { return !(*this == b); }

SymBitVec SymBitVec::maj3(const SymBitVec &a, const SymBitVec &b, const SymBitVec &c) {
  if ((a.size() != b.size()) || (a.size() != c.size())) {
    char err_msg[128];
    snprintf(err_msg, 128, "Bit vectors must be same size, got (%u, %u, %u)", a.size(),
             b.size(), c.size());
    throw std::length_error(err_msg);
  }

  std::vector<SymBit> bits = {};
  for (unsigned int i = 0; i < a.size(); i++) {
    bits.push_back(SymBit::maj3(a.at(i), b.at(i), c.at(i)));
  }
  return SymBitVec(bits);
}

SymBitVec SymBitVec::xor3(const SymBitVec &a, const SymBitVec &b, const SymBitVec &c) {
  if ((a.size() != b.size()) || (a.size() != c.size())) {
    char err_msg[128];
    snprintf(err_msg, 128, "Bit vectors must be same size, got (%u, %u, %u)", a.size(),
             b.size(), c.size());
    throw std::length_error(err_msg);
  }

  std::vector<SymBit> bits = {};
  for (unsigned int i = 0; i < a.size(); i++) {
    bits.push_back(SymBit::xor3(a.at(i), b.at(i), c.at(i)));
  }
  return SymBitVec(bits);
}

}  // end namespace preimage
