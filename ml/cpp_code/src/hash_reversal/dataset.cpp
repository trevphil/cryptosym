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
#include <iostream>
#include <spdlog/spdlog.h>

#include "hash_reversal/dataset.hpp"

namespace hash_reversal {

Dataset::Dataset(const utils::Config &config) {
  spdlog::info("Loading dataset...");
  train_.reserve(config.num_rvs);
  test_.reserve(config.num_rvs);

  const size_t num_train_bytes = config.num_train_samples / 8;
  const size_t num_test_bytes = config.num_test_samples / 8;

  std::ifstream data(config.data_file, std::ios::in | std::ios::binary);
  char c;

  for (int rv = 0; rv < config.num_rvs; ++rv) {
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

  for (size_t i = 0; i < 32; ++i) {
    std::cout << test_[1][i] << " ";
  }
  std::cout << std::endl;

  data.close();
  spdlog::info("Finished loading dataset.");
}

}  // end namespace hash_reversal