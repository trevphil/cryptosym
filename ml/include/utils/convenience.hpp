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
#include <string>
#include <algorithm>
#include <random>
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

  template<typename T>
  static std::string vec2str(const std::vector<T> &vec) {
    auto begin = vec.begin();
    auto end = vec.end();
    std::stringstream ss;
    ss << "[";
    bool first = true;
    for (; begin != end; ++begin) {
      if (!first) ss << ", ";
      ss << *begin;
      first = false;
    }
    ss << "]";
    return ss.str();
  }

  template<typename T>
  static std::vector<T> randomSubset(const std::vector<T> &input_vec, size_t k) {
    if (k < 0 || k >= input_vec.size()) return input_vec;
    std::vector<unsigned int> indices(input_vec.size());
    std::iota(indices.begin(), indices.end(), 0);
    auto rng = std::default_random_engine{};
    std::shuffle(indices.begin(), indices.end(), rng);
    std::vector<T> output_vec;
    for (size_t i = 0; i < k; ++i) output_vec.push_back(input_vec.at(indices.at(i)));
    return output_vec;
  }
};

}  // end namespace utils