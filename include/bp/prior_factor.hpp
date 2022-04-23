/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <memory>

#include "bp/node.hpp"

namespace preimage {

namespace bp {

class GraphPriorFactor : public GraphFactor {
 public:
  GraphPriorFactor(int i, bool bit) : GraphFactor(i, BPFactorType::Prior), bit_(bit) {}

  virtual ~GraphPriorFactor() {
    for (auto &e : edges_) e.reset();
  }

  void initMessages() override {
    assert(edges_.size() == 1);
    std::shared_ptr<GraphEdge> e = edges_.at(0);
    e->m2n(0) = (bit_ == false ? 1.0 : 0.0);
    e->m2n(1) = (bit_ == true ? 1.0 : 0.0);
  }

  void factor2node() override {
    // Do nothing: message is already out
  }

 private:
  bool bit_;
};

}  // end namespace bp

}  // end namespace preimage