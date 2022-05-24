/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#pragma once

#include "core/sym_bit_vec.hpp"
#include "core/sym_hash.hpp"

namespace preimage {

class RIPEMD160 : public SymHash {
 public:
  RIPEMD160(int num_input_bits, int difficulty = -1);

  SymBitVec forward(const SymBitVec &hash_input) override;

  int defaultDifficulty() const override { return 160; }

  std::string hashName() const override { return "RIPEMD160"; }

 private:
  void resetState();

  void finalize(const SymBitVec &hash_input, int bit_index, int lo, int hi);

  static inline SymBitVec F(const SymBitVec &x, const SymBitVec &y, const SymBitVec &z) {
    return SymBitVec::xor3(x, y, z);
  }

  static inline SymBitVec G(const SymBitVec &x, const SymBitVec &y, const SymBitVec &z) {
    return (z ^ (x & (y ^ z)));
  }

  static inline SymBitVec H(const SymBitVec &x, const SymBitVec &y, const SymBitVec &z) {
    return (z ^ (x | ~y));
  }

  static inline SymBitVec I(const SymBitVec &x, const SymBitVec &y, const SymBitVec &z) {
    return (y ^ (z & (x ^ y)));
  }

  static inline SymBitVec J(const SymBitVec &x, const SymBitVec &y, const SymBitVec &z) {
    return (x ^ (y | ~z));
  }

  static inline SymBitVec rotateLeft(const SymBitVec &x, int n) {
    return (x << n) | (x >> (32 - n));
  }

  void transformInternal(SymBitVec &a1, SymBitVec &a2, SymBitVec &b1, SymBitVec &b2,
                         SymBitVec &c1, SymBitVec &c2, SymBitVec &d1, SymBitVec &d2,
                         SymBitVec &e1, SymBitVec &e2);

  void transform();

  SymBitVec buffer_[5];  // Each SymBitVec in `buffer_` has 32 bits
  SymBitVec X_[16];      // Each SymBitVec in `X_` has 32 bits
  SymBitVec k0_, k1_, k2_, k3_, k4_, k5_, k6_, k7_, k8_, k9_;
};

}  // end namespace preimage
