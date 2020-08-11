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

#include <string>

#include "hash_reversal/inference_tool.hpp"

namespace hash_reversal {

class BayesNet : public InferenceTool {
 public:
  BayesNet(std::shared_ptr<Probability> prob,
           std::shared_ptr<Dataset> dataset,
           std::shared_ptr<utils::Config> config);

  void update(const VariableAssignments &observed) override;

  std::vector<InferenceTool::Prediction> marginals() const override;

 protected:
  void reset() override;
};

}  // end namespace hash_reversal