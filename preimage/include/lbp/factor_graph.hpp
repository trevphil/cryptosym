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
#include <vector>

#include "solver.hpp"
#include "lbp/probability.hpp"
#include "lbp/params.hpp"
#include "lbp/lbp_factor.hpp"

namespace preimage {

namespace lbp {

class FactorGraph : public Solver {
 public:
  FactorGraph(const std::map<size_t, Factor> &factors,
              const std::vector<size_t> &input_indices);

 protected:
  void reset() override;

  std::map<size_t, bool> solveInternal() override;

 private:
  struct Prediction {
    Prediction(size_t i, double p) : rv_index(i), prob_one(p) {}
    size_t rv_index;
    double prob_one;
  };

  Prediction predict(size_t rv_index) const;

  std::vector<Prediction> marginals() const;

  void updateFactorMessages(bool forward);

  void updateRandomVariableMessages(bool forward);

  bool equal(const std::vector<Prediction> &marginals1,
             const std::vector<Prediction> &marginals2) const;

  std::map<size_t, RandomVariable> rvs_;
  std::map<size_t, LbpFactor> lbp_factors_;
  std::vector<Prediction> previous_marginals_;
  LbpParams params_;
  Probability prob_;
};

}  // end namespace lbp

}  // end namespace preimage
