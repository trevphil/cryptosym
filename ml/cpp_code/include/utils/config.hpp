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

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <yaml-cpp/yaml.h>

#include <ctime>
#include <string>
#include <iostream>
#include <filesystem>
#include <vector>
#include <memory>

namespace utils {

class Config {
 public:
  explicit Config(std::string config_file);

  size_t lbp_max_iter;
  std::string hash_algo;
  std::string dataset_dir;
  std::string data_file;
  std::string graph_file;
  double epsilon;
  size_t num_rvs;
  size_t num_samples;
  size_t num_train_samples;
  size_t num_test_samples;
  size_t num_hash_bits;
  size_t num_input_bits;
  size_t num_internal_bits;

 private:
  void configureLogging() const;

  void loadYAML(const std::string &config_file);

  void loadDatasetParameters();

  void validateParameters() const;
};

}  // end namespace utils