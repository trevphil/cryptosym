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

#include <set>
#include <string>

namespace hash_reversal {

struct RandomVariable {
  std::set<size_t> factor_indices;
};

struct Factor {
  std::string factor_type;
  size_t primary_rv;
  std::set<size_t> rv_dependencies;

  std::set<size_t> referencedRVs() const {
    std::set<size_t> referenced = rv_dependencies;
    referenced.insert(primary_rv);
    return referenced;
  }
};

} // end namespace hash_reversal