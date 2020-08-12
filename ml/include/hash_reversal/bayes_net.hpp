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

#include <gtsam/discrete/Signature.h>
#include <gtsam/discrete/DiscreteKey.h>
#include <gtsam/discrete/DiscreteFactorGraph.h>
#include <gtsam/discrete/DiscreteMarginals.h>
#include <gtsam/inference/BayesNet.h>

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

 private:
  std::vector<InferenceTool::Prediction> predictions_;
  gtsam::DiscreteBayesNet dbn_;
  gtsam::DiscreteFactorGraph factor_graph_;
  gtsam::Ordering ordering_;
};

}  // end namespace hash_reversal