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

#include "hash_funcs.hpp"

#include "utils.hpp"

namespace dataset_generator {

SymBitVec LossyPseudoHash::hash(const SymBitVec &hash_input, int difficulty) {
  const size_t n = hash_input.size();
  const size_t n4 = n / 4;

  const SymBitVec A(0xDEADBEEF12345678, n);
  const SymBitVec B(0xFADBADB00BEAD321, n);
  const SymBitVec C(0x1123579A00423CDF, n);
  const SymBitVec D(0x0987654321FEDCBA, n);

  const SymBitVec mask((1 << n4) - 1, n);
  SymBitVec h = hash_input;

  for (int i = 0; i < difficulty; i++) {
    SymBitVec a = ((h >> (n4 * 0)) & mask) ^ A;
    SymBitVec b = ((h >> (n4 * 1)) & mask) ^ B;
    SymBitVec c = ((h >> (n4 * 2)) & mask) ^ C;
    SymBitVec d = ((h >> (n4 * 3)) & mask) ^ D;
    SymBitVec e = (a | b);
    SymBitVec f = (b & c);
    SymBitVec g = (c ^ d);
    h = e | (f << (n4 * 1)) | (g << (n4 * 2)) | (d << (n4 * 3));
  }

  return h;
}

}  // end namespace dataset_generator