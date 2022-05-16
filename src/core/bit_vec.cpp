/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include "core/bit_vec.hpp"

#include <stdexcept>

namespace preimage {

BitVec::BitVec() : bits_() {}

BitVec::BitVec(const std::string &bit_str) : bits_(bit_str.size()) {
  const unsigned int n = bit_str.size();
  for (unsigned int i = 0; i < n; ++i) {
    const char c = bit_str[n - i - 1];
    if (c == '0') {
      bits_[i] = false;
    } else if (c == '1') {
      bits_[i] = true;
    } else {
      char err_msg[128];
      snprintf(err_msg, 128, "BitVec found char '%c' in binary string", c);
      throw std::invalid_argument(err_msg);
    }
  }
}

BitVec::BitVec(unsigned int n) : bits_(n) {}

BitVec::BitVec(unsigned int n, unsigned long long data) : bits_(n) {
  for (unsigned int i = 0; i < n; ++i) {
    bits_[i] = static_cast<bool>((data >> i) & 1);
  }
}

BitVec::~BitVec() {}

std::string BitVec::toString() const {
  std::string out = "";
  const unsigned int n = size();
  for (unsigned int i = 0; i < n; ++i) {
    out += bits_.at(n - i - 1) ? '1' : '0';
  }
  return out;
}

}  // end namespace preimage
