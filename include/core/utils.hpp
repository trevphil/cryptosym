/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/bit_vec.hpp"

namespace preimage {

namespace utils {

void seed(unsigned int s);

BitVec zeroBits(unsigned int n);

BitVec randomBits(unsigned int n);

BitVec randomBits(unsigned int n, unsigned int s);

BitVec str2bits(const std::string &s);

std::string hexstr(const BitVec &bs);

BitVec hex2bits(const std::string &hex_str);

std::string binstr(const BitVec &bs);

std::chrono::system_clock::rep ms_since_epoch();

}  // end namespace utils

}  // end namespace preimage
