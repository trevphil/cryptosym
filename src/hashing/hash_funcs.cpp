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

#include "hashing/hash_funcs.hpp"

#include <spdlog/spdlog.h>

#include "core/utils.hpp"

namespace preimage {

SymBitVec SameIOHash::hash(const SymBitVec &hash_input, int difficulty) {
  (void)difficulty;
  return hash_input;
}

SymBitVec NotHash::hash(const SymBitVec &hash_input, int difficulty) {
  (void)difficulty;
  return ~(~(~hash_input));
}

SymBitVec LossyPseudoHash::hash(const SymBitVec &hash_input, int difficulty) {
  const int n = hash_input.size();
  const int n4 = n / 4;

  const SymBitVec A(0xDEADBEEF12345678, n);
  const SymBitVec B(0xFADBADB00BEAD321, n);
  const SymBitVec C(0x1123579A00423CDF, n);
  const SymBitVec D(0x0987654321FEDCBA, n);

  const SymBitVec mask((1 << n4) - 1, n);
  SymBitVec h = hash_input;
  SymBitVec a, b, c, d;

  for (int i = 0; i < difficulty; i++) {
    a = ((h >> (n4 * 0)) & mask) ^ A;
    b = ((h >> (n4 * 1)) & mask) ^ B;
    c = ((h >> (n4 * 2)) & mask) ^ C;
    d = ((h >> (n4 * 3)) & mask) ^ D;
    a = (a | b);
    b = (b & c);
    c = (c ^ d);
    h = a | (b << (n4 * 1)) | (c << (n4 * 2)) | (d << (n4 * 3));
  }

  return h;
}

SymBitVec NonLossyPseudoHash::hash(const SymBitVec &hash_input, int difficulty) {
  const int n = hash_input.size();
  const int n4 = n / 4;

  const SymBitVec A(0xDEADBEEF12345678, n);
  const SymBitVec B(0xFADBADB00BEAD321, n);
  const SymBitVec C(0x1123579A00423CDF, n);
  const SymBitVec D(0x0987654321FEDCBA, n);

  const SymBitVec mask((1 << n4) - 1, n);
  SymBitVec h = hash_input;
  SymBitVec a, b, c, d;

  for (int i = 0; i < difficulty; i++) {
    a = ((h >> (n4 * 0)) & mask) ^ A;
    b = ((h >> (n4 * 1)) & mask) ^ B;
    c = ((h >> (n4 * 2)) & mask) ^ C;
    d = ((h >> (n4 * 3)) & mask) ^ D;
    h = a | (b << (n4 * 1)) | (c << (n4 * 2)) | (d << (n4 * 3));
  }

  return h;
}
}  // end namespace preimage