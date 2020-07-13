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

namespace hash_reversal {

struct VariableAssignment {
  VariableAssignment(size_t rv_idx, bool v) : rv_index(rv_idx), value(v) {}
  size_t rv_index;
  bool value;
};

}  // end namespace hash_reversal