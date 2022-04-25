/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <boost/dynamic_bitset.hpp>
#include <string>

#include "core/sym_bit_vec.hpp"
#include "core/sym_representation.hpp"

namespace preimage {

class SymHash {
 public:
  SymHash(int num_input_bits, int difficulty = -1);

  virtual ~SymHash();

  boost::dynamic_bitset<> call(const boost::dynamic_bitset<> &hash_input);

  boost::dynamic_bitset<> callRandom();

  SymRepresentation getSymbolicRepresentation();

  double averageRuntimeMs() const;

  virtual int defaultDifficulty() const = 0;

  virtual std::string hashName() const = 0;

 protected:
  virtual SymBitVec hash(const SymBitVec &hash_input) = 0;

 protected:
  int num_input_bits_;
  int difficulty_;

 private:
  double num_calls_, cum_runtime_ms_;
};

}  // end namespace preimage
