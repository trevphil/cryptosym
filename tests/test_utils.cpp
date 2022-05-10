/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <gtest/gtest.h>

#include <boost/dynamic_bitset.hpp>
#include <string>

#include "core/utils.hpp"

namespace preimage {

TEST(UtilsTest, Conversions) {
  const boost::dynamic_bitset<> bits(16, 0b1101001100011101);
  const std::string hex = "d31d";
  const std::string bin = "1101001100011101";
  EXPECT_EQ(utils::hexstr(bits), hex);
  EXPECT_EQ(utils::binstr(bits), bin);
  EXPECT_EQ(utils::hex2bits(hex), bits);

  boost::dynamic_bitset<> deadbeef(64, 0xDEADBEEF);
  EXPECT_EQ(utils::hexstr(deadbeef), "00000000deadbeef");

  const boost::dynamic_bitset<> cheese(48, 0x657365656863);
  EXPECT_EQ(utils::str2bits("cheese"), cheese);
}

TEST(UtilsTest, BadHexString) {
  EXPECT_THROW({ utils::hex2bits("wxyz"); }, std::domain_error);
}

TEST(UtilsTest, ZeroBits) {
  const boost::dynamic_bitset<> b = utils::zeroBits(32);
  for (int i = 0; i < 32; ++i) EXPECT_EQ(b[i], 0);
}

TEST(UtilsTest, RandomBits) {
  const int s = 42;  // seed
  const boost::dynamic_bitset<> b1 = utils::randomBits(64, s);
  const boost::dynamic_bitset<> b2 = utils::randomBits(64, s);
  EXPECT_EQ(b1, b2);
  const boost::dynamic_bitset<> b3 = utils::randomBits(64);
  EXPECT_NE(b1, b3);
  utils::seed(s);
  const boost::dynamic_bitset<> b4 = utils::randomBits(64);
  EXPECT_EQ(b1, b4);
}

}  // end namespace preimage
