/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace preimage {

class BitVec {
 public:
  BitVec() : bits_() {}

  explicit BitVec(const std::string &bit_str) : bits_(bit_str.size()) {
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

  explicit BitVec(unsigned int n) : bits_(n) {}

  BitVec(unsigned int n, unsigned long long data) : bits_(n) {
    for (unsigned int i = 0; i < n; ++i) {
      bits_[i] = static_cast<bool>((data >> i) & 1);
    }
  }

  virtual ~BitVec() {}

  inline unsigned int size() const { return bits_.size(); }

  inline std::vector<bool>::reference operator[](unsigned int index) {
    return bits_[index];
  }

  inline bool operator[](unsigned int index) const { return bits_.at(index); }

  inline bool operator==(const BitVec &other) const {
    if (size() != other.size()) return false;
    for (unsigned int i = 0; i < size(); ++i) {
      if (bits_.at(i) != other[i]) return false;
    }
    return true;
  }

  std::string toString() const {
    std::string out = "";
    const unsigned int n = size();
    for (unsigned int i = 0; i < n; ++i) {
      out += bits_.at(n - i - 1) ? '1' : '0';
    }
    return out;
  }

 private:
  std::vector<bool> bits_;
};

}  // end namespace preimage
