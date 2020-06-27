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

#include <fstream>
#include <spdlog/spdlog.h>

#include "hash_reversal/dataset.hpp"

namespace hash_reversal {

Dataset::Dataset(const utils::Config &config) : config_(config) {
  spdlog::info("Loading dataset...");
  const auto start = utils::Convenience::time_since_epoch();
  train_.reserve(config.num_rvs);
  test_.reserve(config.num_rvs);

  const size_t num_train_bytes = config.num_train_samples / 8;
  const size_t num_test_bytes = config.num_test_samples / 8;

  std::ifstream data(config.data_file, std::ios::in | std::ios::binary);
  char c;

  for (size_t rv = 0; rv < config.num_rvs; ++rv) {
    boost::dynamic_bitset<> train_bits(config.num_train_samples);
    boost::dynamic_bitset<> test_bits(config.num_test_samples);

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

std::map<size_t, bool> Dataset::getHashBits(size_t test_sample_index) const {
  std::map<size_t, bool> observed;
  for (size_t hash_bit_idx = 0; hash_bit_idx < config_.num_hash_bits; ++hash_bit_idx) {
    observed[hash_bit_idx] = test_[hash_bit_idx][test_sample_index];
  }
  return observed;
}

bool Dataset::getGroundTruth(size_t test_sample_index) const {
  return test_[config_.bit_to_predict][test_sample_index];
}

}  // end namespace hash_reversal