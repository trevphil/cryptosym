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

#include <utility>
#include <vector>

#include "core/factor.hpp"

namespace preimage {

class Bit {
 public:
  Bit(bool bit_val, bool rv, int d);

  virtual ~Bit();

  static void reset();

  Bit operator~() const;

  Bit operator&(const Bit &b) const;

  Bit operator^(const Bit &b) const;

  Bit operator|(const Bit &b) const;

  static std::pair<Bit, Bit> add(const Bit &a, const Bit &b,
                                 const Bit &carry_in);
  static std::pair<Bit, Bit> add(const Bit &a, const Bit &b);

  static Bit majority3(const Bit &a, const Bit &b, const Bit &c);

  static size_t global_index;
  static std::vector<Bit> global_bits;

  bool val;
  bool is_rv;
  int depth;
  size_t index;
};

}  // end namespace preimage
