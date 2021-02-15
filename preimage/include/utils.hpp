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

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <cstdlib>
#include <string>

namespace preimage {

class Utils {
 public:
  static void seed(unsigned int s) { std::srand(s); }

  static boost::dynamic_bitset<> randomBits(size_t n) {
    boost::dynamic_bitset<> x(n);
    for (size_t i = 0; i < n; ++i) x[i] = rand() % 2;
    return x;
  }

  static boost::dynamic_bitset<> randomBits(size_t n, unsigned int s) {
    seed(s);
    return randomBits(n);
  }

  static boost::dynamic_bitset<> str2bits(const std::string &s) {
    const size_t n = s.size();
    const size_t n_bits = n * 8;
    boost::dynamic_bitset<> bits(n_bits);
    for (size_t i = 0; i < n; ++i) {
      char c = s[i];
      for (size_t j = 0; j < 8; j++) {
        bits[(8 * i) + j] = (c >> j) & 1;
      }
    }
    return bits;
  }

  static std::string hexstr(const boost::dynamic_bitset<> &bs) {
    std::string bitset_str;
    boost::to_string(bs, bitset_str);
    std::string b(bitset_str);

    std::string out = "";

    for (size_t i = 0; i < b.size(); i += 4) {
      int8_t n = 0;
      for (size_t j = i; j < i + 4; ++j) {
        n <<= 1;
        if (b[j] == '1') n |= 1;
      }

      out.push_back(n <= 9 ? ('0' + n) : ('a' + n - 10));
    }

    return out;
  }

  static std::string binstr(const boost::dynamic_bitset<> &bs) {
    std::string bitset_str;
    boost::to_string(bs, bitset_str);
    return bitset_str;
  }
};

}  // end namespace preimage