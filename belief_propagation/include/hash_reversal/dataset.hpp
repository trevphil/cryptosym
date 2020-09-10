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
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "hash_reversal/factor.hpp"
#include "hash_reversal/variable_assignments.hpp"
#include "utils/config.hpp"
#include "utils/convenience.hpp"

namespace hash_reversal {

class Dataset {
 public:
  explicit Dataset(std::shared_ptr<utils::Config> config);

  typedef std::pair<std::map<size_t, RandomVariable>, std::map<size_t, Factor>> Graph;
  Graph loadFactorGraph() const;

  bool isHashInputBit(size_t bit_index) const;

  std::string getHashInput(size_t sample_index) const;

  VariableAssignments getObservedData(size_t sample_index) const;

  boost::dynamic_bitset<> getFullSample(size_t sample_index) const;

  bool validate(const boost::dynamic_bitset<> predicted_input, size_t sample_index) const;

 private:
  std::shared_ptr<utils::Config> config_;
  std::vector<boost::dynamic_bitset<>> samples_;
};

}  // end namespace hash_reversal