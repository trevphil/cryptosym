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

#include <boost/dynamic_bitset.hpp>
#include <memory>
#include <vector>

#include "hash_reversal/variable_assignment.hpp"
#include "utils/config.hpp"
#include "utils/convenience.hpp"

namespace hash_reversal {

class Dataset {
 public:
  explicit Dataset(std::shared_ptr<utils::Config> config);

  std::vector<size_t> hashInputBitIndices() const;

  bool isHashInputBit(size_t bit_index) const;

  std::vector<VariableAssignment> getHashBits(size_t test_sample_index) const;

  boost::dynamic_bitset<> getGroundTruth(size_t test_sample_index) const;

  boost::dynamic_bitset<> getTrainSamples(size_t rv_index) const;

 private:
  std::shared_ptr<utils::Config> config_;
  std::vector<boost::dynamic_bitset<>> train_;
  std::vector<boost::dynamic_bitset<>> test_;
};

}  // end namespace hash_reversal