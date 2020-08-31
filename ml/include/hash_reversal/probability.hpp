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

#include <spdlog/spdlog.h>

#include <boost/dynamic_bitset.hpp>
#include <memory>
#include <vector>

#include "hash_reversal/factor.hpp"
#include "hash_reversal/variable_assignments.hpp"
#include "utils/config.hpp"

namespace hash_reversal {

class Probability {
 public:
  explicit Probability(std::shared_ptr<utils::Config> config);

  double probOne(const Factor &factor, const VariableAssignments &assignments,
                 const VariableAssignments &observed) const;

 private:
  std::shared_ptr<utils::Config> config_;
};

}  // end namespace hash_reversal