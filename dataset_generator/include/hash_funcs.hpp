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

#include "sym_bit_vec.hpp"
#include "sym_hash.hpp"

namespace dataset_generator {

class LossyPseudoHash : public SymHash {
  SymBitVec hash(const SymBitVec &hash_input, int difficulty) override;
};

}  // end namespace dataset_generator