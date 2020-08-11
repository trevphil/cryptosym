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

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <ctime>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "utils/convenience.hpp"

namespace utils {

class Config {
 public:
  explicit Config(std::string config_file);

  bool valid() const;

  size_t lbp_max_iter;
  double lbp_damping;
  std::string hash_algo;
  std::string dataset_dir;
  std::string data_file;
  std::string graph_file;
  std::string method;
  double epsilon;
  size_t num_rvs;
  std::vector<size_t> input_rv_indices;
  std::vector<size_t> hash_rv_indices;
  bool print_connections;
  bool test_mode;
  size_t num_samples;
  size_t num_test;
  size_t difficulty;
  size_t observed_input_bits;

 private:
  bool valid_;

  void configureLogging() const;

  void loadYAML(const std::string &config_file);

  void loadDatasetParameters();

  void validateParameters();
};

}  // end namespace utils