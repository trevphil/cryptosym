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

#include "utils/config.hpp"

namespace utils {

Config::Config(std::string config_file) : valid_(true) {
  configureLogging();
  loadYAML(config_file);
  if (valid_) loadDatasetParameters();
  if (valid_) validateParameters();
  if (test_mode) spdlog::set_level(spdlog::level::err);
}

bool Config::valid() const { return valid_; }

void Config::configureLogging() const {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(&tm, "logs/%Y-%m-%d-%H-%M-%S.log");

  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
  sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(oss.str()));
  auto logger = std::make_shared<spdlog::logger>("basic_logger", begin(sinks), end(sinks));
  spdlog::register_logger(logger);

  spdlog::set_level(spdlog::level::debug);
}

void Config::loadYAML(const std::string &config_file) {
  std::filesystem::path config_path = config_file;
  if (config_path.is_relative()) {
    config_path = std::filesystem::current_path() / config_path;
  }

  if (!std::filesystem::exists(config_path)) {
    spdlog::error("Config file '{}' does not exist", config_path.string());
    valid_ = false;
    return;
  }

  spdlog::info("Loading config from '{}'", config_path.string());
  YAML::Node data = YAML::LoadFile(config_path.string());

  std::string param;

  param = "lbp_max_iter";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    lbp_max_iter = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, lbp_max_iter);
  }

  param = "lbp_damping";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    lbp_damping = data[param].as<double>();
    spdlog::info("{} --> {}", param, lbp_damping);
  }

  param = "dataset_dir";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    dataset_dir = data[param].as<std::string>();
    spdlog::info("{} --> {}", param, dataset_dir);
  }

  param = "epsilon";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    epsilon = data[param].as<double>();
    spdlog::info("{} --> {}", param, epsilon);
  }

  param = "num_test";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    num_test = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, num_test);
  }

  param = "print_connections";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    print_connections = data[param].as<bool>();
    spdlog::info("{} --> {}", param, print_connections);
  }

  param = "print_bit_accuracies";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    print_bit_accuracies = data[param].as<bool>();
    spdlog::info("{} --> {}", param, print_bit_accuracies);
  }

  param = "test_mode";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    test_mode = data[param].as<bool>();
    spdlog::info("{} --> {}", param, test_mode);
  }
}

void Config::loadDatasetParameters() {
  std::filesystem::path dataset_base = dataset_dir;
  std::filesystem::path dataset_params = dataset_base / "params.yaml";
  std::filesystem::path dataset_data = dataset_base / "data.bits";
  std::filesystem::path dataset_graph = dataset_base / "factors.txt";
  std::vector<std::filesystem::path> paths = {dataset_params, dataset_data, dataset_graph};

  for (auto &p : paths) {
    if (p.is_relative()) {
      p = std::filesystem::current_path() / p;
    }

    if (!std::filesystem::exists(p)) {
      spdlog::error("'{}' does not exist", p.string());
      valid_ = false;
      return;
    }
  }

  data_file = dataset_data.string();
  graph_file = dataset_graph.string();
  spdlog::info("dataset params --> {}", dataset_params.string());
  spdlog::info("dataset bits --> {}", data_file);
  spdlog::info("dataset factor graph --> {}", graph_file);

  YAML::Node data = YAML::LoadFile(dataset_params.string());

  std::string param;

  param = "hash";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    hash_algo = data[param].as<std::string>();
    spdlog::info("{} --> {}", param, hash_algo);
  }

  param = "num_rvs";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    num_rvs = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, num_rvs);
  }

  param = "num_samples";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    num_samples = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, num_samples);
  }

  param = "hash_rv_indices";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    hash_rv_indices = data[param].as<std::vector<size_t>>();
    spdlog::info("{} --> {}", param, utils::Convenience::vec2str<size_t>(hash_rv_indices));
  }

  param = "input_rv_indices";
  if (!data[param]) {
    valid_ = false;
    spdlog::error("Missing '{}'", param);
  } else {
    input_rv_indices = data[param].as<std::vector<size_t>>();
    spdlog::info("{} --> {}", param, utils::Convenience::vec2str<size_t>(input_rv_indices));
  }
}

void Config::validateParameters() {
  if (num_samples % 8 != 0) {
    valid_ = false;
    spdlog::error("Number of samples is not a multiple of 8");
  }
}

}  // end namespace utils