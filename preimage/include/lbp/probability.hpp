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

#include <map>

#include "lbp/params.hpp"
#include "lbp/lbp_factor.hpp"

namespace preimage {

namespace lbp {

class Probability {
 public:
  explicit Probability(const LbpParams &params);

  double probOne(const LbpFactor &factor,
                 const std::map<size_t, bool> &assignments,
                 const std::map<size_t, bool> &observed) const;

 private:
  LbpParams params_;
};

}  // end namespace lbp

}  // end namespace preimage