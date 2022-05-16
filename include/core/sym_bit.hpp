/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#pragma once

#include <utility>

namespace preimage {

class SymBit {
 public:
  SymBit(bool bit_val, bool is_unknown);

  virtual ~SymBit();

  static void reset();

  static SymBit zero();

  static SymBit one();

  static SymBit constant(bool val);

  SymBit operator~() const;

  SymBit operator&(const SymBit &b) const;

  SymBit operator^(const SymBit &b) const;

  SymBit operator|(const SymBit &b) const;

  static std::pair<SymBit, SymBit> add(const SymBit &a, const SymBit &b,
                                       const SymBit &carry_in);
  static std::pair<SymBit, SymBit> add(const SymBit &a, const SymBit &b);

  static SymBit majority3(const SymBit &a, const SymBit &b, const SymBit &c);

  static SymBit xor3(const SymBit &a, const SymBit &b, const SymBit &c);

  thread_local static int global_index;

  bool val;
  int index;
  bool unknown;
};

}  // end namespace preimage
