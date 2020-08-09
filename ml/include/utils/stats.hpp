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

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>

#include <spdlog/spdlog.h>

#include "utils/config.hpp"
#include "utils/convenience.hpp"

namespace utils {

class Stats {
 public:
  Stats(std::shared_ptr<Config> config, const std::vector<std::string> &factor_types)
      : config_(config), factor_types_(factor_types) {
    const size_t n = config_->num_rvs;
    num_correct_per_rv_ = std::vector<double>(n, 0);
    rv_cumulative_val_ = std::vector<double>(n, 0);
    count_per_rv_ = std::vector<double>(n, 0);
  }

  void update(size_t rv_index, bool predicted_val, bool true_val,
              double prob_one, bool is_observed) {
    const bool is_correct = (predicted_val == true_val);

    if (!is_observed) {
      (is_correct ? correct_pred_ : incorrect_pred_).push_back(prob_one);
    }

    num_correct_per_rv_[rv_index] += is_correct;
    const std::string f_type = factor_types_.at(rv_index);
    num_correct_per_factor_[f_type] += is_correct;
    rv_cumulative_val_[rv_index] += true_val;
    count_per_rv_[rv_index] += 1;
    count_per_factor_[f_type] += 1;
  }

  void save() const {
    const std::string filename = "statistics.txt";
    std::ofstream data(filename);

    data << "probability bit is one for correct predictions" << std::endl;
    data << Convenience::vec2str<double>(correct_pred_, false) << std::endl;

    data << "probability bit is one for incorrect predictions" << std::endl;
    data << Convenience::vec2str<double>(incorrect_pred_, false) << std::endl;

    data << "bit accuracies" << std::endl;
    for (size_t rv = 0; rv < config_->num_rvs; ++rv) {
      data << rv << "," << num_correct_per_rv_.at(rv) / count_per_rv_.at(rv);
      data << std::endl;
    }

    data << "factor accuracies" << std::endl;
    for (auto it : num_correct_per_factor_) {
      data << it.first << "," << it.second / count_per_factor_.at(it.first);
      data << std::endl;
    }

    data << "bit mean values" << std::endl;
    for (size_t rv = 0; rv < config_->num_rvs; ++rv) {
      data << rv << "," << rv_cumulative_val_.at(rv) / count_per_rv_.at(rv);
      data << std::endl;
    }

    data.close();
    spdlog::info("Statistics were written to '{}'", filename);
  }

 private:
  std::shared_ptr<Config> config_;

  //! Each RV at index `i` has an associated factor of some type (XOR, AND, ...)
  std::vector<std::string> factor_types_;

  //! Tracks "probability the bit is one" for each (in)correct bit prediction.
  //  Observed bits (hash output bits) are excluded because they're always correct!
  std::vector<double> correct_pred_, incorrect_pred_;

  //! Counts the number of correct predictions for each random variable
  std::vector<double> num_correct_per_rv_;

  //! Counts the number of correct predictions for each factor type
  std::map<std::string, double> num_correct_per_factor_;

  //! Cumulative sum of the value of each random variable when it is seen
  std::vector<double> rv_cumulative_val_;

  //! Number of times each random variable is seen
  std::vector<double> count_per_rv_;

  //! Number of times each factor type is seen
  std::map<std::string, double> count_per_factor_;
};

}  // end namespace utils