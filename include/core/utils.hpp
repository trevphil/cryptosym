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
#include <array>
#include <boost/dynamic_bitset.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace preimage {

class Utils {
 public:
  static void seed(unsigned int s) { std::srand(s); }

  static boost::dynamic_bitset<> randomBits(int n) {
    boost::dynamic_bitset<> x(n);
    for (int i = 0; i < n; ++i) x[i] = rand() % 2;
    return x;
  }

  static boost::dynamic_bitset<> randomBits(int n, unsigned int s) {
    seed(s);
    return randomBits(n);
  }

  static boost::dynamic_bitset<> str2bits(const std::string &s) {
    const int n = s.size();
    const int n_bits = n * 8;
    boost::dynamic_bitset<> bits(n_bits);
    for (int i = 0; i < n; ++i) {
      char c = s[i];
      for (int j = 0; j < 8; j++) {
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

    for (int i = 0; i < b.size(); i += 4) {
      int8_t n = 0;
      for (int j = i; j < i + 4; ++j) {
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

  static std::chrono::system_clock::rep ms_since_epoch() {
    static_assert(std::is_integral<std::chrono::system_clock::rep>::value,
                  "Representation of ticks isn't an integral value.");

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  }

  static std::chrono::system_clock::rep sec_since_epoch() {
    static_assert(std::is_integral<std::chrono::system_clock::rep>::value,
                  "Representation of ticks isn't an integral value.");

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
  }

  template <typename T>
  static std::string vec2str(const std::vector<T> &v, bool brackets = true) {
    auto begin = v.begin();
    auto end = v.end();
    std::stringstream ss;
    if (brackets) ss << "[";
    bool first = true;
    for (; begin != end; ++begin) {
      if (!first) ss << ", ";
      ss << *begin;
      first = false;
    }
    if (brackets) ss << "]";
    return ss.str();
  }

  template <typename T>
  static std::string set2str(const std::set<T> &s, bool brackets = true) {
    auto begin = s.begin();
    auto end = s.end();
    std::stringstream ss;
    if (brackets) ss << "[";
    bool first = true;
    for (; begin != end; ++begin) {
      if (!first) ss << ", ";
      ss << *begin;
      first = false;
    }
    if (brackets) ss << "]";
    return ss.str();
  }

  static std::string exec(const std::string &cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");

    while (std::fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
      result += buffer.data();

    return result;
  }
};

}  // end namespace preimage