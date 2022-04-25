/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

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

namespace utils {

void seed(unsigned int s) { std::srand(s); }

boost::dynamic_bitset<> zeroBits(int n) {
  boost::dynamic_bitset<> x(n);
  for (int i = 0; i < n; ++i) x[i] = 0;
  return x;
}

boost::dynamic_bitset<> randomBits(int n) {
  boost::dynamic_bitset<> x(n);
  for (int i = 0; i < n; ++i) x[i] = rand() % 2;
  return x;
}

boost::dynamic_bitset<> randomBits(int n, unsigned int s) {
  seed(s);
  return randomBits(n);
}

boost::dynamic_bitset<> str2bits(const std::string &s) {
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

std::string hexstr(const boost::dynamic_bitset<> &bs) {
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

const char *hex2bin(char c) {
  switch (std::toupper(c)) {
    case '0':
      return "0000";
    case '1':
      return "0001";
    case '2':
      return "0010";
    case '3':
      return "0011";
    case '4':
      return "0100";
    case '5':
      return "0101";
    case '6':
      return "0110";
    case '7':
      return "0111";
    case '8':
      return "1000";
    case '9':
      return "1001";
    case 'A':
      return "1010";
    case 'B':
      return "1011";
    case 'C':
      return "1100";
    case 'D':
      return "1101";
    case 'E':
      return "1110";
    case 'F':
      return "1111";
    default:
      assert(false);
      return "0000";
  }
}

boost::dynamic_bitset<> hex2bits(const std::string &hex_str) {
  std::string bin_str;
  const int n = static_cast<int>(hex_str.length());
  for (int i = 0; i < n; ++i) bin_str += hex2bin(hex_str[i]);
  return boost::dynamic_bitset<>(bin_str);
}

std::string binstr(const boost::dynamic_bitset<> &bs) {
  std::string bitset_str;
  boost::to_string(bs, bitset_str);
  return bitset_str;
}

std::chrono::system_clock::rep ms_since_epoch() {
  static_assert(std::is_integral<std::chrono::system_clock::rep>::value,
                "Representation of ticks isn't an integral value.");

  const auto now = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

}  // end namespace utils

}  // end namespace preimage