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

#include "hash_reversal/factor_graph.hpp"

namespace hash_reversal {

FactorGraph::FactorGraph(std::shared_ptr<Probability> prob,
                         std::shared_ptr<utils::Config> config) {
  spdlog::info("Loading adjacency matrix from CSV...");
  const auto start = utils::Convenience::time_since_epoch();

  const size_t n = config->num_rvs;
  std::ifstream data;
  data.open(config->graph_file);
  std::string line;
  adjacency_mat_ = Eigen::MatrixXf(n, n);

  int row = 0;
  while (std::getline(data, line)) {
    std::stringstream line_stream(line);
    std::string cell;
    int col = 0;
    while (std::getline(line_stream, cell, ',')) {
      adjacency_mat_(row, col++) = std::stof(cell);
    }
    ++row;
  }

  data.close();

  const auto end = utils::Convenience::time_since_epoch();
  spdlog::info("Finished loading adjacency matrix in {} seconds.", end - start);
}

FactorGraph::Prediction FactorGraph::predict(size_t bit_index, bool bit_value,
                                             const std::map<size_t, bool> &observed) {
  FactorGraph::Prediction prediction;
  prediction.log_likelihood_ratio = 0.1;
  prediction.prob_bit_is_one = 0.6;
  return prediction;  // TODO
}

}  // end namespace hash_reversal