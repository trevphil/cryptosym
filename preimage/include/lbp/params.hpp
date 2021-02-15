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

namespace preimage {

namespace lbp {

struct LbpParams {
 public:
  size_t max_iter = 50;
  double damping = 0.75;
  double epsilon = 0.0001;
};

}  // end namespace lbp

}  // end namespace preimage
