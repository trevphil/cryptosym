/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
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

namespace utils {

void seed(unsigned int s);

boost::dynamic_bitset<> zeroBits(int n);

boost::dynamic_bitset<> randomBits(int n);

boost::dynamic_bitset<> randomBits(int n, unsigned int s);

boost::dynamic_bitset<> str2bits(const std::string &s);

std::string hexstr(const boost::dynamic_bitset<> &bs);

boost::dynamic_bitset<> hex2bits(const std::string &hex_str);

std::string binstr(const boost::dynamic_bitset<> &bs);

std::chrono::system_clock::rep ms_since_epoch();

}  // end namespace utils

}  // end namespace preimage
