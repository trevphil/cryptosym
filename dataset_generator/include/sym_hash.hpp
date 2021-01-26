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

#include "sym_bit_vec.hpp"

namespace dataset_generator {

class SymHash {
 public:
  SymHash();

  virtual ~SymHash();

  std::vector<size_t> hashInputIndices() const;

  std::vector<size_t> hashOutputIndices() const;

  size_t numUnknownsPerHash() const;

  size_t numUsefulFactors();

  void saveFactors(const std::string &factor_filename,
                   const std::string &cnf_filename);

  virtual SymBitVec hash(const SymBitVec &hash_input, int difficulty);

  SymBitVec call(const boost::dynamic_bitset<> &hash_input, int difficulty);

  void findIgnorableRVs();

 private:
  std::set<size_t> ignorable_;
  bool did_find_ignorable_;
  std::vector<size_t> hash_input_indices_;
  std::vector<size_t> hash_output_indices_;
};

}  // end namespace dataset_generator
