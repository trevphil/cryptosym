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
#include <gtsam/nonlinear/LevenbergMarquardtOptimizer.h>
#include <gtsam/nonlinear/NonlinearFactor.h>
#include <gtsam/nonlinear/NonlinearFactorGraph.h>
#include <gtsam/nonlinear/PriorFactor.h>
#include <gtsam/nonlinear/Values.h>

#include <string>
#include <vector>

#include "hash_reversal/inference_tool.hpp"

namespace hash_reversal {

class SameFactor : public gtsam::NoiseModelFactor2<double, double> {
 public:
  SameFactor(gtsam::Key k1, gtsam::Key k2, const gtsam::SharedNoiseModel &model)
      : gtsam::NoiseModelFactor2<double, double>(model, k1, k2) {}

  gtsam::Vector evaluateError(const double &inp, const double &out,
                              boost::optional<gtsam::Matrix &> J1 = boost::none,
                              boost::optional<gtsam::Matrix &> J2 = boost::none) const {
    // if (H) (*H) = (Matrix(2,3)<< 1.0,0.0,0.0, 0.0,1.0,0.0).finished();
    // return (Vector(2) << q.x() - mx_, q.y() - my_).finished();
    if (J1) (*J1) = (gtsam::Matrix(1, 1) << 2 * inp - 2 * out).finished();
    if (J2) (*J2) = (gtsam::Matrix(1, 1) << 2 * out - 2 * inp).finished();
    const double err = inp - out;
    return (gtsam::Vector(1) << err * err).finished();
  }
};

class InvFactor : public gtsam::NoiseModelFactor2<double, double> {
 public:
  InvFactor(gtsam::Key k1, gtsam::Key k2, const gtsam::SharedNoiseModel &model)
      : gtsam::NoiseModelFactor2<double, double>(model, k1, k2) {}

  gtsam::Vector evaluateError(const double &inp, const double &out,
                              boost::optional<gtsam::Matrix &> J1 = boost::none,
                              boost::optional<gtsam::Matrix &> J2 = boost::none) const {
    if (J1) (*J1) = (gtsam::Matrix(1, 1) << 2 * inp + 2 * out - 2).finished();
    if (J2) (*J2) = (gtsam::Matrix(1, 1) << 2 * out + 2 * inp - 2).finished();
    const double err = 1.0 - inp - out;
    return (gtsam::Vector(1) << err * err).finished();
  }
};

class AndFactor : public gtsam::NoiseModelFactor3<double, double, double> {
 public:
  AndFactor(gtsam::Key k1, gtsam::Key k2, gtsam::Key k3,
            const gtsam::SharedNoiseModel &model, size_t c)
      : gtsam::NoiseModelFactor3<double, double, double>(model, k1, k2, k3), case_(c) {}

  gtsam::Vector evaluateError(const double &inp1, const double &inp2, const double &out,
                              boost::optional<gtsam::Matrix &> J1 = boost::none,
                              boost::optional<gtsam::Matrix &> J2 = boost::none,
                              boost::optional<gtsam::Matrix &> J3 = boost::none) const {
    double err, i1_partial, i2_partial, out_partial;
    if (case_ == 0) {
      // inp1 = 0, inp2 = 0, out = 0
      err = inp1 * inp1 + inp2 * inp2 + out * out;
      i1_partial = 2 * inp1;
      i2_partial = 2 * inp2;
      out_partial = 2 * out;
    } else if (case_ == 1) {
      // inp1 = 0, inp2 = 1, out = 0
      err = inp1 * inp1 + (1 - inp2) * (1 - inp2) + out * out;
      i1_partial = 2 * inp1;
      i2_partial = 2 * (inp2 - 1);
      out_partial = 2 * out;
    } else if (case_ == 2) {
      // inp1 = 1, inp2 = 0, out = 0
      err = (1 - inp1) * (1 - inp1) + inp2 * inp2 + out * out;
      i1_partial = 2 * (inp1 - 1);
      i2_partial = 2 * inp2;
      out_partial = 2 * out;
    } else if (case_ == 3) {
      // inp1 = 1, inp2 = 1, out = 1
      err = (1 - inp1) * (1 - inp1) + (1 - inp2) * (1 - inp2) + (1 - out) * (1 - out);
      i1_partial = 2 * (inp1 - 1);
      i2_partial = 2 * (inp2 - 1);
      out_partial = 2 * (out - 1);
    } else if (case_ == 4) {
      err = (inp1 * inp2 - out) * (inp1 * inp2 - out);
      i1_partial = 2 * inp1 * inp2 * inp2 - 2 * out * inp2;
      i2_partial = 2 * inp1 * inp1 * inp2 - 2 * out * inp1;
      out_partial = 2 * out - 2 * inp1 * inp2;
    }
    if (J1) (*J1) = (gtsam::Matrix(1, 1) << i1_partial).finished();
    if (J2) (*J2) = (gtsam::Matrix(1, 1) << i2_partial).finished();
    if (J3) (*J3) = (gtsam::Matrix(1, 1) << out_partial).finished();
    return (gtsam::Vector(1) << err).finished();
  }

 private:
  size_t case_;
};

class BayesNet : public InferenceTool {
 public:
  BayesNet(std::shared_ptr<Probability> prob, std::shared_ptr<Dataset> dataset,
           std::shared_ptr<utils::Config> config);

  void solve() override;

  std::vector<InferenceTool::Prediction> marginals() const override;

 protected:
  void reconfigure(const VariableAssignments &observed) override;

 private:
  std::vector<InferenceTool::Prediction> predictions_;
  gtsam::Values init_values_;
  gtsam::NonlinearFactorGraph graph_;
  gtsam::noiseModel::Base::shared_ptr high_noise_;
  gtsam::noiseModel::Base::shared_ptr low_noise_;
  gtsam::noiseModel::Base::shared_ptr dcs_noise_;
};

}  // end namespace hash_reversal