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
#include <iostream>

namespace hash_reversal {

Dataset::Dataset(std::shared_ptr<utils::Config> config) : config_(config) {
  spdlog::info("Loading dataset...");
  const auto start = utils::Convenience::time_since_epoch();

  const size_t n_samples = config->num_samples;
  const size_t bps = config->num_bits_per_sample;
  samples_.reserve(n_samples);
  for (size_t i = 0; i < n_samples; ++i) {
    samples_.push_back(boost::dynamic_bitset<>(bps));
  }

  std::ifstream data(config->data_file, std::ios::in | std::ios::binary);
  char c;
  size_t i = 0;

  while (i < n_samples * bps) {
    data.get(c);
    for (size_t j = 0; j < 8; ++j) {
      const bool bit_val = (c >> (8 - j)) & 1;
      const size_t sample_idx = i / bps;
      const size_t rv_idx = i - (sample_idx * bps);
      samples_.at(sample_idx)[rv_idx] = bit_val;
      ++i;
    }
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

  std::map<size_t, RandomVariable> rvs;
  std::map<size_t, Factor> factors;

  while (std::getline(data, line)) {
    std::stringstream line_stream(line);
    std::string tmp;

    std::getline(line_stream, tmp, ';');
    const std::string factor_type = tmp;

    std::getline(line_stream, tmp, ';');
    const size_t output_rv = std::stoul(tmp);
    std::set<size_t> referenced_rvs = {output_rv};
    rvs[output_rv].factor_indices.insert(output_rv);

    while (std::getline(line_stream, tmp, ';')) {
      const size_t input_rv = std::stoul(tmp);
      referenced_rvs.insert(input_rv);
      rvs[input_rv].factor_indices.insert(output_rv);
    }

    if (factor_type == "AND" && referenced_rvs.size() < 3u) {
      spdlog::warn("AND factor may reference the same RV twice as an input");
    }

    factors[output_rv] = Factor(factor_type, output_rv, referenced_rvs);
  }

  data.close();
  return {rvs, factors};
}

bool Dataset::isHashInputBit(size_t bit_index) const {
  // Critical assumption here is that the input bits are at the beginning
  return bit_index < config_->num_input_bits;
}

std::string Dataset::getHashInput(size_t sample_index) const {
  // Critical assumption here is that the input bits are at the beginning
  const size_t n = config_->num_input_bits;
  boost::dynamic_bitset<> input_bits(n);
  for (size_t i = 0; i < n; ++i) {
    input_bits[i] = samples_.at(sample_index)[i];
  }
  return utils::Convenience::bitset2hex(input_bits);
}

VariableAssignments Dataset::getObservedData(size_t sample_index) const {
  VariableAssignments observed;

  for (const size_t &bit_idx : config_->observed_rv_indices) {
    const auto bitval = samples_.at(sample_index)[bit_idx];
    observed[bit_idx] = bitval;
  }

  return observed;
}

bool Dataset::validate(const boost::dynamic_bitset<> predicted_input,
                       size_t sample_index) const {
  const std::string pred_in = utils::Convenience::bitset2hex(predicted_input);
  const std::string true_in = getHashInput(sample_index);
  std::ostringstream cmd;
  cmd << "python -m dataset_generation.generate";
  cmd << " --num-input-bits " << predicted_input.size();
  cmd << " --hash-algo " << config_->hash_algo;
  cmd << " --difficulty " << config_->difficulty;
  cmd << " --hash-input ";

  const std::string pred_hash = utils::Convenience::exec(cmd.str() + pred_in);
  const std::string true_hash = utils::Convenience::exec(cmd.str() + true_in);

  if (pred_hash == true_hash) {
    spdlog::info("\tHashes match: {}", pred_hash, true_hash);
    return true;
  }

  spdlog::info("\tHashes do not match!");
  spdlog::info("\t\tPredicted input {}", pred_in);
  spdlog::info("\t\tPrediction gave {}", pred_hash);
  spdlog::info("\t\tCorrect hash is {}", true_hash);
  return false;
}

boost::dynamic_bitset<> Dataset::getFullSample(size_t sample_index) const {
  const size_t n = config_->num_bits_per_sample;
  boost::dynamic_bitset<> all_bits(n);
  for (size_t i = 0; i < n; ++i) {
    all_bits[i] = samples_.at(sample_index)[i];
  }
  return all_bits;
}

}  // end namespace hash_reversal