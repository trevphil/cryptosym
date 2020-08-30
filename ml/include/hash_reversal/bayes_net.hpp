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

#include <gtsam/inference/Symbol.h>
#include <gtsam/nonlinear/Values.h>
#include <gtsam/nonlinear/PriorFactor.h>
#include <gtsam/nonlinear/NonlinearFactor.h>
#include <gtsam/nonlinear/NonlinearFactorGraph.h>
#include <gtsam/nonlinear/LevenbergMarquardtOptimizer.h>

#include <string>
#include <vector>

#include "hash_reversal/inference_tool.hpp"

namespace hash_reversal {

class InvFactor : public gtsam::NoiseModelFactor2<double, double> {
 public:
  InvFactor(gtsam::Key k1, gtsam::Key k2, const gtsam::SharedNoiseModel &model)
      : gtsam::NoiseModelFactor2<double, double>(model, k1, k2) { }

  gtsam::Vector evaluateError(const double &inp, const double &out,
                              boost::optional<gtsam::Matrix&> H1 = boost::none,
                              boost::optional<gtsam::Matrix&> H2 = boost::none) const {
    // if (H) (*H) = (Matrix(2,3)<< 1.0,0.0,0.0, 0.0,1.0,0.0).finished();
    // return (Vector(2) << q.x() - mx_, q.y() - my_).finished();
    if (H1) (*H1) = (gtsam::Matrix(1, 1) << -1.0).finished();
    if (H2) (*H2) = (gtsam::Matrix(1, 1) << -1.0).finished();
    return (gtsam::Vector(1) << 1.0 - inp - out).finished();
  }
};

class AndFactor : public gtsam::NoiseModelFactor3<double, double, double> {
 public:
  AndFactor(gtsam::Key k1, gtsam::Key k2, gtsam::Key k3,
            const gtsam::SharedNoiseModel &model)
      : gtsam::NoiseModelFactor3<double, double, double>(model, k1, k2, k3) { }

  gtsam::Vector evaluateError(const double &inp1, const double &inp2, const double &out,
                              boost::optional<gtsam::Matrix&> H1 = boost::none,
                              boost::optional<gtsam::Matrix&> H2 = boost::none,
                              boost::optional<gtsam::Matrix&> H3 = boost::none) const {
    // if (H) (*H) = (Matrix(2,3)<< 1.0,0.0,0.0, 0.0,1.0,0.0).finished();
    // return (Vector(2) << q.x() - mx_, q.y() - my_).finished();
    if (H1) (*H1) = (gtsam::Matrix(1, 1) << inp2).finished();
    if (H2) (*H2) = (gtsam::Matrix(1, 1) << inp1).finished();
    if (H3) (*H3) = (gtsam::Matrix(1, 1) << -1.0).finished();
    return (gtsam::Vector(1) << inp1 * inp2 - out).finished();
  }
};

class BayesNet : public InferenceTool {
 public:
  BayesNet(std::shared_ptr<Probability> prob,
           std::shared_ptr<Dataset> dataset,
           std::shared_ptr<utils::Config> config);

  void solve() override;

  std::vector<InferenceTool::Prediction> marginals() const override;

 protected:
  void reconfigure(const VariableAssignments &observed) override;

 private:
  std::vector<InferenceTool::Prediction> predictions_;
  gtsam::Values init_values_;
  gtsam::NonlinearFactorGraph graph_;
  gtsam::noiseModel::Diagonal::shared_ptr noise_;
  gtsam::noiseModel::Diagonal::shared_ptr prior_noise_;
};

}  // end namespace hash_reversal