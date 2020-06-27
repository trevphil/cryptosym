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

#include <vector>
#include <boost/dynamic_bitset.hpp>

#include "utils/config.hpp"

namespace hash_reversal {

class Dataset {
 public:
	explicit Dataset(const utils::Config &config);

 private:
  std::vector<boost::dynamic_bitset<>> train_;
  std::vector<boost::dynamic_bitset<>> test_;
};

}  // end namespace hash_reversal