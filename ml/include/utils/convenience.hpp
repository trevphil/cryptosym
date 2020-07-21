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

#include <algorithm>
#include <chrono>
#include <set>
#include <string>
#include <vector>

namespace utils {

class Convenience {
 public:
  static std::chrono::system_clock::rep time_since_epoch() {
    static_assert(std::is_integral<std::chrono::system_clock::rep>::value,
                  "Representation of ticks isn't an integral value.");

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
  }

  template <typename T>
  static std::string vec2str(const std::vector<T> &v) {
    auto begin = v.begin();
    auto end = v.end();
    std::stringstream ss;
    ss << "[";
    bool first = true;
    for (; begin != end; ++begin) {
      if (!first) ss << ", ";
      ss << *begin;
      first = false;
    }
    ss << "]";
    return ss.str();
  }

  template <typename T>
  static std::string set2str(const std::set<T> &s) {
    auto begin = s.begin();
    auto end = s.end();
    std::stringstream ss;
    ss << "[";
    bool first = true;
    for (; begin != end; ++begin) {
      if (!first) ss << ", ";
      ss << *begin;
      first = false;
    }
    ss << "]";
    return ss.str();
  }

  static std::string bitset2hex(const boost::dynamic_bitset<> &bs) {
    std::string bitset_str;
    boost::to_string(bs, bitset_str);
    std::string b(bitset_str);
    std::reverse(b.begin(), b.end());

    std::string out;

    for (size_t i = 0; i < b.size(); i += 4) {
      int8_t n = 0;
      for (size_t j = i; j < i + 4; ++j) {
        n <<= 1;
        if (b[j] == '1') n |= 1;
      }

      if (n <= 9) out.push_back('0' + n);
      else out.push_back('a' + n - 10);
    }

    return out;
  }
};

}  // end namespace utils