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

Dataset::Graph Dataset::loadFactorGraph() const {
  std::ifstream data;
  data.open(config_->graph_file);
  std::string line;

  const size_t n = config_->num_rvs;
  std::vector<RandomVariable> rvs(n);
  std::vector<Factor> factors;
  factors.reserve(n);
  for (size_t i = 0; i < n; ++i) {
    factors.push_back(Factor(i));
    rvs[i].factor_indices.insert(i);
  }

  while (std::getline(data, line)) {
    std::stringstream line_stream(line);
    std::string tmp;

    std::getline(line_stream, tmp, ';');
    std::string factor_type = tmp;

    std::getline(line_stream, tmp, ';');
    const size_t rv_index = std::stoul(tmp);
    factors[rv_index].factor_type = factor_type;

    while (std::getline(line_stream, tmp, ';')) {
      const size_t rv_dependency = std::stoul(tmp);
      factors[rv_index].referenced_rvs.insert(rv_dependency);
      rvs[rv_dependency].factor_indices.insert(rv_index);
    }
  }

  data.close();
  return {rvs, factors};
}

bool Dataset::isHashInputBit(size_t bit_index) const {
  for (const size_t &idx : config_->hash_rv_indices) {
    if (bit_index == idx) return true;
  }
  return false;
}

VariableAssignments Dataset::getObservedData(size_t sample_index) const {
  VariableAssignments observed;

  for (const size_t &bit_idx : config_->hash_rv_indices) {
    const auto bitval = samples_.at(bit_idx)[sample_index];
    observed[bit_idx] = bitval;
  }

  return observed;
}

boost::dynamic_bitset<> Dataset::getFullSample(size_t sample_index) const {
  const size_t n = config_->num_rvs;
  boost::dynamic_bitset<> all_bits(n);
  for (size_t i = 0; i < n; ++i) {
    all_bits[i] = samples_.at(i)[sample_index];
  }
  return all_bits;
}

}  // end namespace hash_reversal