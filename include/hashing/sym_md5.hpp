/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#pragma once

#include <vector>

#include "core/sym_bit_vec.hpp"
#include "core/sym_hash.hpp"

#define MD5_BLOCK_SIZE 64

namespace preimage {

class MD5 : public SymHash {
 public:
  MD5(int num_input_bits, int difficulty = -1);

  SymBitVec hash(const SymBitVec &hash_input) override;

  int defaultDifficulty() const override { return 64; }

  std::string hashName() const override { return "MD5"; }

 private:
  void init();

  void finalize();

  void decode(std::vector<SymBitVec> &output, const std::vector<SymBitVec> &input,
              int len);

  void encode(std::vector<SymBitVec> &output, const std::vector<SymBitVec> &input,
              int len);

  void update(const std::vector<SymBitVec> &input, int len);

  void transform(const std::vector<SymBitVec> &block);

  void transformInternal(const std::vector<SymBitVec> &block, SymBitVec &a, SymBitVec &b,
                         SymBitVec &c, SymBitVec &d);

  static inline SymBitVec F(const SymBitVec &x, const SymBitVec &y, const SymBitVec &z) {
    return (x & y) | (~x & z);
  }

  static inline SymBitVec G(const SymBitVec &x, const SymBitVec &y, const SymBitVec &z) {
    return (x & z) | (y & ~z);
  }

  static inline SymBitVec H(const SymBitVec &x, const SymBitVec &y, const SymBitVec &z) {
    return SymBitVec::xor3(x, y, z);
  }

  static inline SymBitVec I(const SymBitVec &x, const SymBitVec &y, const SymBitVec &z) {
    return y ^ (x | ~z);
  }

  static inline SymBitVec rotateLeft(const SymBitVec &x, int n) {
    return (x << n) | (x >> (32 - n));
  }

  static inline void FF(SymBitVec &a, const SymBitVec &b, const SymBitVec &c,
                        const SymBitVec &d, const SymBitVec &x, uint32_t s,
                        const SymBitVec &ac) {
    a = rotateLeft(a + F(b, c, d) + x + ac, s) + b;
  }

  static inline void GG(SymBitVec &a, const SymBitVec &b, const SymBitVec &c,
                        const SymBitVec &d, const SymBitVec &x, uint32_t s,
                        const SymBitVec &ac) {
    a = rotateLeft(a + G(b, c, d) + x + ac, s) + b;
  }

  static inline void HH(SymBitVec &a, const SymBitVec &b, const SymBitVec &c,
                        const SymBitVec &d, const SymBitVec &x, uint32_t s,
                        const SymBitVec &ac) {
    a = rotateLeft(a + H(b, c, d) + x + ac, s) + b;
  }

  static inline void II(SymBitVec &a, const SymBitVec &b, const SymBitVec &c,
                        const SymBitVec &d, const SymBitVec &x, uint32_t s,
                        const SymBitVec &ac) {
    a = rotateLeft(a + I(b, c, d) + x + ac, s) + b;
  }

  bool finalized;

  // Bytes that didn't fit into last 64-byte chunk.
  // Each SymBitVec is 8 bits.
  std::vector<SymBitVec> buffer;

  // 64-bit counter for number of bits (lo, hi).
  uint32_t count[2];

  // Digest so far. Each SymBitVec is 32 bits.
  std::vector<SymBitVec> state;

  // Final digest result. Each SymBitVec is 8 bits.
  std::vector<SymBitVec> digest;

  // Known constants
  std::vector<SymBitVec> constants_;
};

}  // end namespace preimage
