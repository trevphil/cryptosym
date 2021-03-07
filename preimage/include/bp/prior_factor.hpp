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

#include "bp/node.hpp"

namespace preimage {

namespace bp {

class GraphPriorFactor : public GraphFactor {
 public:
  GraphPriorFactor(size_t i, bool bit)
      : GraphFactor(i, BPFactorType::Prior), bit_(bit) {}

  virtual ~GraphPriorFactor() {
    for (auto &e : edges_) e.reset();
  }

  void factor2node() override {
    std::shared_ptr<GraphEdge> e = edges_.at(0);
    e->m2n(0) = (bit_ == false ? 1.0 : 0.0);
    e->m2n(1) = (bit_ == true ? 1.0 : 0.0);
  }

 private:
  bool bit_;
};

}  // end namespace bp

}  // end namespace preimage