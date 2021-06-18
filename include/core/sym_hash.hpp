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

#include <boost/dynamic_bitset.hpp>

#include <string>
#include <set>

#include "core/sym_bit_vec.hpp"

namespace preimage {

class SymHash {
 public:
  SymHash();

  virtual ~SymHash();

  std::vector<int> hashInputIndices() const;

  std::vector<int> hashOutputIndices() const;

  int numUsefulFactors();

  int dagDepth() const;

  SymBitVec call(const boost::dynamic_bitset<> &hash_input,
                 int difficulty = -1);

  bool canIgnore(int rv);

  double averageRuntimeMs() const;

  virtual int defaultDifficulty() const = 0;

  virtual std::string hashName() const = 0;

 protected:
  virtual SymBitVec hash(const SymBitVec &hash_input, int difficulty) = 0;

 private:
  void findIgnorableRVs();

  int numUnknownsPerHash() const;

  std::set<int> ignorable_;
  bool did_find_ignorable_;
  std::vector<int> hash_input_indices_;
  std::vector<int> hash_output_indices_;
  double num_calls_, cum_runtime_ms_;
};

}  // end namespace preimage
