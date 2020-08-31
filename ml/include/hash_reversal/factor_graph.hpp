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

class FactorGraph : public InferenceTool {
 public:
  FactorGraph(std::shared_ptr<Probability> prob, std::shared_ptr<Dataset> dataset,
              std::shared_ptr<utils::Config> config);

  void solve() override;

  std::vector<InferenceTool::Prediction> marginals() const override;

 protected:
  void reconfigure(const VariableAssignments &observed) override;

 private:
  Prediction predict(size_t rv_index) const;
  void updateFactorMessages(bool forward);
  void updateRandomVariableMessages(bool forward);
  bool equal(const std::vector<InferenceTool::Prediction> &marginals1,
             const std::vector<InferenceTool::Prediction> &marginals2,
             double tol = 1e-4) const;

  std::vector<InferenceTool::Prediction> previous_marginals_;
};

}  // end namespace hash_reversal