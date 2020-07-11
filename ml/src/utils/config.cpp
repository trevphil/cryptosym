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

Config::Config(std::string config_file) {
  configureLogging();
  loadYAML(config_file);
  loadDatasetParameters();
  validateParameters();
}

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

std::string Config::graphVizFile(size_t count) const {
  std::ostringstream oss;
  oss << "./viz/grap_viz_" << count << ".xml";
  return oss.str();
}

void Config::loadYAML(const std::string &config_file) {
  std::filesystem::path config_path = config_file;
  if (config_path.is_relative()) {
    config_path = std::filesystem::current_path() / config_path;
  }

  if (!std::filesystem::exists(config_path)) {
    spdlog::error("Config file '{}' does not exist", config_path.string());
    return;
  }

  spdlog::info("Loading config from '{}'", config_path.string());
  YAML::Node data = YAML::LoadFile(config_path.string());

  std::string param;

  param = "lbp_max_iter";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    lbp_max_iter = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, lbp_max_iter);
  }

  param = "dataset_dir";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    dataset_dir = data[param].as<std::string>();
    spdlog::info("{} --> {}", param, dataset_dir);
  }

  param = "epsilon";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    epsilon = data[param].as<double>();
    spdlog::info("{} --> {}", param, epsilon);
  }

  param = "print_connections";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    print_connections = data[param].as<bool>();
    spdlog::info("{} --> {}", param, print_connections);
  }
}

void Config::loadDatasetParameters() {
  std::filesystem::path dataset_base = dataset_dir;
  std::filesystem::path dataset_params = dataset_base / "params.yaml";
  std::filesystem::path dataset_data = dataset_base / "data.bits";
  std::filesystem::path dataset_graph = dataset_base / "graph.csv";
  std::vector<std::filesystem::path> paths = {
    dataset_params, dataset_data, dataset_graph };

  for (auto &p : paths) {
    if (p.is_relative()) {
      p = std::filesystem::current_path() / p;
    }

    if (!std::filesystem::exists(p)) {
      spdlog::error("'{}' does not exist", p.string());
      return;
    }
  }

  data_file = dataset_data.string();
  graph_file = dataset_graph.string();
  spdlog::info("dataset params --> {}", dataset_params.string());
  spdlog::info("dataset bits --> {}", data_file);
  spdlog::info("dataset graph --> {}", graph_file);

  YAML::Node data = YAML::LoadFile(dataset_params.string());

  std::string param;

  param = "hash";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    hash_algo = data[param].as<std::string>();
    spdlog::info("{} --> {}", param, hash_algo);
  }

  param = "num_rvs";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    num_rvs = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, num_rvs);
  }

  param = "num_samples";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    num_samples = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, num_samples);
  }

  param = "num_train_samples";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    num_train_samples = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, num_train_samples);
  }

  param = "num_test_samples";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    num_test_samples = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, num_test_samples);
  }

  param = "num_hash_bits";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    num_hash_bits = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, num_hash_bits);
  }

  param = "num_input_bits";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    num_input_bits = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, num_input_bits);
  }

  param = "num_internal_bits";
  if (!data[param]) {
    spdlog::error("Missing '{}'", param);
  } else {
    num_internal_bits = data[param].as<size_t>();
    spdlog::info("{} --> {}", param, num_internal_bits);
  }
}

void Config::validateParameters() const {
  if (num_samples != num_train_samples + num_test_samples)
    spdlog::error("Total number of samples should equal # train + # test samples");
  if (num_rvs != num_hash_bits + num_input_bits + num_internal_bits)
    spdlog::error("Number of RVs should equal # hash bits + # input bits + # internals");
  if (num_samples % 8 != 0)
    spdlog::error("Number of samples is not a multiple of 8");
  if (num_train_samples % 8 != 0)
    spdlog::error("Number of train samples is not a multiple of 8");
  if (num_test_samples % 8 != 0)
    spdlog::error("Number of test samples is not a multiple of 8");
}

}  // end namespace utils