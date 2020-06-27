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

#include "hash_reversal/dataset.hpp"

namespace hash_reversal {

class Probability {
 public:
	explicit Probability(const Dataset &dataset);
};

}  // end namespace hash_reversal