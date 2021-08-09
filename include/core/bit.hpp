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

#include "core/logic_gate.hpp"

namespace preimage {

class Bit {
 public:
  Bit(bool bit_val, bool is_unknown, int dpth);

  virtual ~Bit();

  static void reset();

  static Bit zero();

  static Bit one();

  static Bit constant(bool val);

  Bit operator~() const;

  Bit operator&(const Bit &b) const;

  Bit operator^(const Bit &b) const;

  Bit operator|(const Bit &b) const;

  static std::pair<Bit, Bit> add(const Bit &a, const Bit &b,
                                 const Bit &carry_in);
  static std::pair<Bit, Bit> add(const Bit &a, const Bit &b);

  static Bit majority3(const Bit &a, const Bit &b, const Bit &c);

  static Bit xor3(const Bit &a, const Bit &b, const Bit &c);

  static int global_index;

  bool val;
  int index;
  bool unknown;
  int depth;
};

}  // end namespace preimage