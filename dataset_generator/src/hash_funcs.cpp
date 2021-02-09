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

  // Utils::seed(1);
  const SymBitVec A(Utils::randomBits(n, 3));
  const SymBitVec B(Utils::randomBits(n, 7));
  const SymBitVec C(Utils::randomBits(n, 11));
  const SymBitVec D(Utils::randomBits(n, 13));

  const SymBitVec mask((1 << n4) - 1, n);
  SymBitVec h = hash_input;

  for (int i = 0; i < difficulty; i++) {
    SymBitVec a = ((h >> (n4 * 0)) & mask) ^ A;
    SymBitVec b = ((h >> (n4 * 1)) & mask) ^ B;
    SymBitVec c = ((h >> (n4 * 2)) & mask) ^ C;
    SymBitVec d = ((h >> (n4 * 3)) & mask) ^ D;
    a = (a | b);
    b = (b & c);
    c = (c ^ d);
    h = a | (b << (n4 * 1)) | (c << (n4 * 2)) | (d << (n4 * 3));
  }

  return h;
}

}  // end namespace dataset_generator