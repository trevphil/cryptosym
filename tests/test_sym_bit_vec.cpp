/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <gtest/gtest.h>

#include <boost/dynamic_bitset.hpp>
#include <string>

#include "core/sym_bit_vec.hpp"

namespace preimage {

TEST(SymBitVecTest, Conversions) {
  const boost::dynamic_bitset<> bits_a(16, 0b1101001100011101);
  const SymBitVec bv(bits_a);
  const std::string hex = "d31d";
  const std::string bin = "1101001100011101";
  EXPECT_EQ(bv.bits(), bits_a);
  EXPECT_EQ(bv.intVal(), 0b1101001100011101);
  EXPECT_EQ(bv.bin(), bin);
  EXPECT_EQ(bv.hex(), hex);

  boost::dynamic_bitset<> bits_b(64, 0xDEADBEEF);
  SymBitVec bv1(bits_b);
  SymBitVec bv2(0xDEADBEEF, 64);
  EXPECT_EQ(bv1.bits(), bits_b);
  EXPECT_EQ(bv2.bits(), bits_b);
  EXPECT_EQ(bv1.intVal(), uint64_t(0xDEADBEEF));
  EXPECT_EQ(bv2.intVal(), uint64_t(0xDEADBEEF));
  EXPECT_EQ(bv1.hex(), "00000000deadbeef");
  EXPECT_EQ(bv2.hex(), "00000000deadbeef");
}

TEST(SymBitVecTest, BasicOperators) {
  SymBitVec bv1(0b110101, 6);
  SymBitVec bv2(0b011101, 6);
  EXPECT_EQ(bv1.reversed().intVal(), 0b101011);
  EXPECT_EQ(bv2.reversed().intVal(), 0b101110);
  EXPECT_EQ(bv1.rotr(2).intVal(), 0b010111);
  EXPECT_EQ((bv1 >> 3).intVal(), 0b000110);
  EXPECT_EQ((bv1 << 3).intVal(), 0b101000);
  EXPECT_EQ((~bv1).intVal(), 0b001010);
  EXPECT_EQ((bv1 ^ bv2).intVal(), 0b101000);
  EXPECT_EQ((bv1 & bv2).intVal(), 0b010101);
  EXPECT_EQ((bv1 | bv2).intVal(), 0b111101);
  EXPECT_EQ((bv1 + bv2).intVal(), 0b010010);
  EXPECT_EQ(bv1.at(0).val, 1);  // LSB
  EXPECT_EQ(bv2.at(0).val, 1);  // LSB
  EXPECT_EQ(bv1.at(5).val, 1);  // MSB
  EXPECT_EQ(bv2.at(5).val, 0);  // MSB
}

TEST(SymBitVecTest, Resizing) {
  SymBitVec bv1(0b110101, 6);
  SymBitVec bv2(0b011101, 6);
  EXPECT_EQ(bv1.extract(1, 5).intVal(), 0b1010);
  SymBitVec bv1_bigger = bv1.resize(10);
  EXPECT_EQ(bv1_bigger.size(), 10);
  EXPECT_EQ(bv1_bigger.intVal(), 0b0000110101);
  SymBitVec bv1_smaller = bv1.resize(2);
  EXPECT_EQ(bv1_smaller.size(), 2);
  EXPECT_EQ(bv1_smaller.intVal(), 0b01);
  SymBitVec bv12 = bv1.concat(bv2);
  EXPECT_EQ(bv12.size(), 12);
  EXPECT_EQ(bv12.intVal(), (0b011101 << 6) + 0b110101);
}

TEST(SymBitVecTest, AdditionWithZero) {
  SymBitVec t0(0, 32);
  SymBitVec t1(0b11010100010010100110100011100000, 32);
  SymBitVec summed = t0 + t1;
  EXPECT_EQ(t1.intVal(), summed.intVal());
}

TEST(SymBitVecTest, ThreeWayXOR) {
  SymBitVec a(0b11010101, 8);
  SymBitVec b(0b10001001, 8);
  SymBitVec c(0b01011111, 8);
  EXPECT_EQ(SymBitVec::xor3(a, b, c).intVal(), 0b00000011);
}

TEST(SymBitVecTest, Majority3) {
  SymBitVec a(0b11010101, 8);
  SymBitVec b(0b10001001, 8);
  SymBitVec c(0b01011111, 8);
  EXPECT_EQ(SymBitVec::majority3(a, b, c).intVal(), 0b11011101);
}

}  // end namespace preimage
