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

#include "utils/config.hpp"
#include "utils/convenience.hpp"
#include "hash_reversal/factor.hpp"
#include "hash_reversal/dataset.hpp"
#include "hash_reversal/probability.hpp"
#include "hash_reversal/variable_assignments.hpp"

namespace hash_reversal {

class FactorGraph {
 public:
  struct Prediction {
    Prediction(size_t i, double p) : rv_index(i), prob_one(p) { }
    size_t rv_index;
    double prob_one;
  };

  explicit FactorGraph(std::shared_ptr<Probability> prob,
                       std::shared_ptr<Dataset> dataset,
                       std::shared_ptr<utils::Config> config);

  void runLBP(const VariableAssignments &observed);

  std::vector<Prediction> marginals() const;

 private:
  Prediction predict(size_t rv_index) const;
  void updateFactorMessages(bool forward, const VariableAssignments &observed);
  void updateRandomVariableMessages(bool forward);
  void printConnections() const;
  bool equal(const std::vector<Prediction> &marginals1,
             const std::vector<Prediction> &marginals2,
             double tol = 1e-4) const;

  std::shared_ptr<Probability> prob_;
  std::shared_ptr<Dataset> dataset_;
  std::shared_ptr<utils::Config> config_;
  std::vector<RandomVariable> rvs_;
  std::vector<Factor> factors_;
  std::vector<Prediction> previous_marginals_;
};

}  // end namespace hash_reversal