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
  train_.reserve(config->num_rvs);
  test_.reserve(config->num_rvs);

  const size_t num_train_bytes = config->num_train_samples / 8;
  const size_t num_test_bytes = config->num_test_samples / 8;

  std::ifstream data(config->data_file, std::ios::in | std::ios::binary);
  char c;

  for (size_t rv = 0; rv < config->num_rvs; ++rv) {
    boost::dynamic_bitset<> train_bits(config->num_train_samples);
    boost::dynamic_bitset<> test_bits(config->num_test_samples);

    // Note that the order of placing bits into `train_bits` and `test_bits`
    // really doesn't matter since the samples are unordered
    for (size_t i = 0; i < num_train_bytes; ++i) {
      data.get(c);
      for (size_t j = 0; j < 8; ++j) train_bits[(i * 8) + j] = ((c >> j) & 1);
    }

    for (size_t i = 0; i < num_test_bytes; ++i) {
      data.get(c);
      for (size_t j = 0; j < 8; ++j) test_bits[(i * 8) + j] = ((c >> j) & 1);
    }

    train_.push_back(train_bits);
    test_.push_back(test_bits);
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

std::vector<VariableAssignment> Dataset::getHashBits(size_t test_sample_index) const {
  std::vector<VariableAssignment> observed;
  for (size_t hash_bit_idx = 0; hash_bit_idx < config_->num_hash_bits; ++hash_bit_idx) {
    const auto bitval = test_.at(hash_bit_idx)[test_sample_index];
    observed.push_back(VariableAssignment(hash_bit_idx, bitval));
  }
  return observed;
}

boost::dynamic_bitset<> Dataset::getGroundTruth(size_t test_sample_index) const {
  boost::dynamic_bitset<> hash_input(config_->num_rvs);
  for (size_t i = 0; i < hash_input.size(); ++i) {
    hash_input[i] = test_.at(i)[test_sample_index];
  }
  return hash_input;
}

boost::dynamic_bitset<> Dataset::getTrainSamples(size_t rv_index) const {
  return train_.at(rv_index);
}

}  // end namespace hash_reversal