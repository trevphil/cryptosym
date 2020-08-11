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

#include "hash_reversal/bayes_net.hpp"

#include <spdlog/spdlog.h>

namespace hash_reversal {

BayesNet::BayesNet(std::shared_ptr<Probability> prob,
                   std::shared_ptr<Dataset> dataset,
                   std::shared_ptr<utils::Config> config)
    : InferenceTool(prob, dataset, config) { }

void BayesNet::update(const VariableAssignments &observed) {
}

std::vector<InferenceTool::Prediction> BayesNet::marginals() const {
  return {};
}

void BayesNet::reset() {
}

}  // end namespace hash_reversal