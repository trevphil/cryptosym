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

#include <chrono>
#include <type_traits>

namespace utils {

class Convenience {
 public:
  static std::chrono::system_clock::rep time_since_epoch() {
    static_assert(
      std::is_integral<std::chrono::system_clock::rep>::value,
      "Representation of ticks isn't an integral value.");

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
  }
};

}  // end namespace utils