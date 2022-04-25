/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <utility>
#include <vector>

#include "core/sym_bit_vec.hpp"
#include "core/sym_hash.hpp"

namespace preimage {

class SHA256 : public SymHash {
 public:
  SHA256(int num_input_bits, int difficulty = -1);

  SymBitVec hash(const SymBitVec &hash_input) override;

  int defaultDifficulty() const override { return 64; }

  std::string hashName() const override { return "SHA256"; }

 private:
  void resetState();
  void update(const SymBitVec &bv);
  void transform();
  SymBitVec digest();

  std::pair<SymBitVec, SymBitVec> round(const SymBitVec &a, const SymBitVec &b,
                                        const SymBitVec &c, const SymBitVec &d,
                                        const SymBitVec &e, const SymBitVec &f,
                                        const SymBitVec &g, const SymBitVec &h, int i,
                                        const SymBitVec &ki);

  int local_, count_lo_, count_hi_;
  int block_size_, digest_size_;
  std::vector<SymBitVec> w_, words_, data_, digest_;
};

}  // end namespace preimage