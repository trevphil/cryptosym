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

#include "hash_reversal/dataset.hpp"

#include <spdlog/spdlog.h>

#include <fstream>

namespace hash_reversal {

Dataset::Dataset(std::shared_ptr<utils::Config> config) : config_(config) {
  spdlog::info("Loading dataset...");
  const auto start = utils::Convenience::time_since_epoch();
  samples_.reserve(config->num_rvs);

  const size_t num_bytes = config->num_samples / 8;

  std::ifstream data(config->data_file, std::ios::in | std::ios::binary);
  char c;

  for (size_t rv = 0; rv < config->num_rvs; ++rv) {
    boost::dynamic_bitset<> sample_bits(config->num_samples);

    for (size_t i = 0; i < num_bytes; ++i) {
      data.get(c);
      for (size_t j = 0; j < 8; ++j) sample_bits[(i * 8) + j] = ((c >> j) & 1);
    }

    samples_.push_back(sample_bits);
  }

  if (data.get(c)) {
    spdlog::warn("Data file was not 100% read, results are likely garbage.");
  } else if (!data.eof()) {
    spdlog::warn("Reading from data file did not reach EOF");
  }

  data.close();

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("Finished loading dataset in {} seconds.", end - start);
}

std::vector<size_t> Dataset::hashInputBitIndices() const {
  std::vector<size_t> indices;
  indices.reserve(config_->num_input_bits);
  for (size_t i = 0; i < config_->num_input_bits; ++i)
    indices.push_back(config_->num_hash_bits + i);
  return indices;
}

bool Dataset::isHashInputBit(size_t bit_index) const {
  const size_t hash_input_lb = config_->num_hash_bits;
  const size_t hash_input_ub = config_->num_hash_bits + config_->num_input_bits;
  return bit_index >= hash_input_lb && bit_index < hash_input_ub;
}

std::vector<VariableAssignment> Dataset::getHashBits(size_t sample_index) const {
  std::vector<VariableAssignment> observed;
  for (size_t hash_bit_idx = 0; hash_bit_idx < config_->num_hash_bits; ++hash_bit_idx) {
    const auto bitval = samples_.at(hash_bit_idx)[sample_index];
    observed.push_back(VariableAssignment(hash_bit_idx, bitval));
  }
  return observed;
}

boost::dynamic_bitset<> Dataset::getGroundTruth(size_t sample_index) const {
  boost::dynamic_bitset<> all_bits(config_->num_rvs);
  for (size_t i = 0; i < all_bits.size(); ++i) {
    all_bits[i] = samples_.at(i)[sample_index];
  }
  return all_bits;
}

}  // end namespace hash_reversal