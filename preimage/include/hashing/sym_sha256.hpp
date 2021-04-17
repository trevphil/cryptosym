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

#include <utility>
#include <vector>

#include "sym_bit_vec.hpp"
#include "hashing/sym_hash.hpp"

namespace preimage {

class SHA256 : public SymHash {
 public:
  SHA256();

  SymBitVec hash(const SymBitVec &hash_input, int difficulty) override;

 private:
  void resetState();
  void update(const SymBitVec &bv, int difficulty);
  void transform(int difficulty);
  SymBitVec digest(int difficulty);

  std::pair<SymBitVec, SymBitVec> round(const SymBitVec &a, const SymBitVec &b,
                                        const SymBitVec &c, const SymBitVec &d,
                                        const SymBitVec &e, const SymBitVec &f,
                                        const SymBitVec &g, const SymBitVec &h,
                                        int i, const SymBitVec &ki);

  size_t local_, count_lo_, count_hi_;
  size_t block_size_, digest_size_;
  std::vector<SymBitVec> w_, words_, data_, digest_;
};

}  // end namespace preimage