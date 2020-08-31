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

#include <memory>
#include <vector>

#include "hash_reversal/dataset.hpp"
#include "hash_reversal/factor.hpp"
#include "hash_reversal/probability.hpp"
#include "hash_reversal/variable_assignments.hpp"
#include "utils/config.hpp"
#include "utils/convenience.hpp"

namespace hash_reversal {

class InferenceTool {
 public:
  struct Prediction {
    Prediction(size_t i, double p) : rv_index(i), prob_one(p) {}
    size_t rv_index;
    double prob_one;
  };

  InferenceTool(std::shared_ptr<Probability> prob, std::shared_ptr<Dataset> dataset,
                std::shared_ptr<utils::Config> config);

  virtual ~InferenceTool();

  virtual void reconfigure(const VariableAssignments &observed);

  virtual void solve();

  virtual std::vector<Prediction> marginals() const;

  std::string factorType(size_t rv_index) const;

 protected:
  std::shared_ptr<Probability> prob_;
  std::shared_ptr<Dataset> dataset_;
  std::shared_ptr<utils::Config> config_;
  std::vector<RandomVariable> rvs_;
  std::vector<Factor> factors_;
  VariableAssignments observed_;

 private:
  void printConnections() const;
};

}  // end namespace hash_reversal
