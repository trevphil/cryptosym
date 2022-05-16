/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <string>
#include <vector>

namespace preimage {

class BitVec {
 public:
  BitVec();

  explicit BitVec(const std::string &bit_str);

  explicit BitVec(unsigned int n);

  BitVec(unsigned int n, unsigned long long data);

  virtual ~BitVec();

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

  std::string toString() const;

 private:
  std::vector<bool> bits_;
};

}  // end namespace preimage
