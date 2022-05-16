/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <gtest/gtest.h>

#include <string>

#include "core/bit_vec.hpp"

namespace preimage {

TEST(BitVecTest, Conversions) {
  const BitVec a(16, 0b1101001100011101);
  const BitVec b("1101001100011101");
  EXPECT_EQ(a.size(), 16);
  EXPECT_EQ(a.size(), b.size());
  EXPECT_EQ(a.toString(), "1101001100011101");
  EXPECT_EQ(b.toString(), "1101001100011101");
  EXPECT_EQ(a, b);
}

TEST(BitVecTest, Equality) {
  BitVec a("1011");
  BitVec b(4, 0b1011);
  BitVec c("1010");
  BitVec d(5, 0b1011);
  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
  EXPECT_NE(a, d);
}

TEST(BitVecTest, Indexing) {
  BitVec a("10110");
  EXPECT_EQ(a[0], 0);
  EXPECT_EQ(a[1], 1);
  EXPECT_EQ(a[2], 1);
  EXPECT_EQ(a[3], 0);
  EXPECT_EQ(a[4], 1);
  a[0] = 1;
  EXPECT_EQ(a[0], 1);
  a[4] = 0;
  EXPECT_EQ(a[4], 0);
}

}  // end namespace preimage
