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

#include "core/sym_bit_vec.hpp"
#include "core/sym_hash.hpp"

namespace preimage {

class RIPEMD160 : public SymHash {
 public:
  RIPEMD160();

  SymBitVec hash(const SymBitVec &hash_input, int difficulty) override;

  int defaultDifficulty() const override { return 160; }

  std::string hashName() const override { return "RIPEMD160"; }

 private:
  void resetState(int difficulty);

  void finalize(const SymBitVec &hash_input, size_t bit_index,
                size_t lo, size_t hi);

  static inline SymBitVec F(const SymBitVec &x, const SymBitVec &y,
                            const SymBitVec &z) {
    return (x ^ y ^ z);
  }

  static inline SymBitVec G(const SymBitVec &x, const SymBitVec &y,
                            const SymBitVec &z) {
    return (z ^ (x & (y ^ z)));
  }

  static inline SymBitVec H(const SymBitVec &x, const SymBitVec &y,
                            const SymBitVec &z) {
    return (z ^ (x | ~y));
  }

  static inline SymBitVec I(const SymBitVec &x, const SymBitVec &y,
                            const SymBitVec &z) {
    return (y ^ (z & (x ^ y)));
  }

  static inline SymBitVec J(const SymBitVec &x, const SymBitVec &y,
                            const SymBitVec &z) {
    return (x ^ (y | ~z));
  }

  static inline SymBitVec rotateLeft(const SymBitVec &x, int n) {
    return (x << n) | (x >> (32 - n));
  }

  void transformInternal(SymBitVec &a1, SymBitVec &a2,
                         SymBitVec &b1, SymBitVec &b2,
                         SymBitVec &c1, SymBitVec &c2,
                         SymBitVec &d1, SymBitVec &d2,
                         SymBitVec &e1, SymBitVec &e2);

  void transform();

  int difficulty_;
  SymBitVec buffer_[5];  // Each SymBitVec in `buffer_` has 32 bits
  SymBitVec X_[16];      // Each SymBitVec in `X_` has 32 bits
  SymBitVec k0_, k1_, k2_, k3_, k4_, k5_, k6_, k7_, k8_, k9_;
};

}  // end namespace preimage
