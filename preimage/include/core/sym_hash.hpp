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
#include <set>

#include "core/sym_bit_vec.hpp"

namespace preimage {

class SymHash {
 public:
  SymHash();

  virtual ~SymHash();

  std::vector<size_t> hashInputIndices() const;

  std::vector<size_t> hashOutputIndices() const;

  size_t numUsefulFactors();

  void saveStatistics(const std::string &stats_filename);

  void saveFactors(const std::string &factor_filename);

  void saveFactorsCNF(const std::string &cnf_filename);

  SymBitVec call(const boost::dynamic_bitset<> &hash_input, int difficulty);

  bool canIgnore(size_t rv);

 protected:
  size_t numUnknownsPerHash() const;

  virtual SymBitVec hash(const SymBitVec &hash_input, int difficulty);

 private:
  void findIgnorableRVs();

  std::set<size_t> ignorable_;
  bool did_find_ignorable_;
  std::vector<size_t> hash_input_indices_;
  std::vector<size_t> hash_output_indices_;
};

}  // end namespace preimage
