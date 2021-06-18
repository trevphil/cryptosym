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

#include <spdlog/spdlog.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace preimage {

class Factor {
 public:
  enum Type : char {
    NotFactor = 'N',
    AndFactor = 'A',
    XorFactor = 'X',
    OrFactor = 'O',
    MajFactor = 'M'
  };

  Factor();

  Factor(Type t, const size_t output, const std::vector<size_t> &inputs = {});

  virtual ~Factor();

  std::string toString() const;

  static void reset();

  static size_t numInputs(Type t);

  static std::unordered_map<size_t, Factor> global_factors;

  Type t;
  size_t output;
  std::vector<size_t> inputs;
  size_t n_inputs;
  bool valid;
};

}  // end namespace preimage
