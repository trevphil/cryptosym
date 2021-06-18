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

#include "core/sym_hash.hpp"
#include "core/bit.hpp"
#include "core/factor.hpp"
#include "core/utils.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>
#include <queue>

namespace preimage {

SymHash::SymHash()
    : ignorable_(), did_find_ignorable_(false),
      hash_output_indices_(), num_calls_(0.0),
      cum_runtime_ms_(0.0) {}

SymHash::~SymHash() {}

std::vector<size_t> SymHash::hashInputIndices() const {
  return hash_input_indices_;
}

std::vector<size_t> SymHash::hashOutputIndices() const {
  return hash_output_indices_;
}

size_t SymHash::numUnknownsPerHash() const { return Bit::global_index; }

size_t SymHash::numUsefulFactors() {
  findIgnorableRVs();
  return Bit::global_bits.size() - ignorable_.size();
}

int SymHash::dagDepth() const {
  int max_d = 0;
  for (Bit &b : Bit::global_bits) max_d = std::max(max_d, b.depth);
  return max_d;
}

double SymHash::averageRuntimeMs() const {
  if (num_calls_ == 0) return -1.0;
  return cum_runtime_ms_ / num_calls_;
}

SymBitVec SymHash::call(const boost::dynamic_bitset<> &hash_input,
                        int difficulty) {
  Bit::reset();
  Factor::reset();
  did_find_ignorable_ = false;
  SymBitVec inp(hash_input, true);
  hash_input_indices_ = inp.rvIndices();
  if (difficulty == -1) difficulty = defaultDifficulty();
  const auto start = Utils::ms_since_epoch();
  SymBitVec out = hash(inp, difficulty);
  cum_runtime_ms_ += Utils::ms_since_epoch() - start;
  num_calls_++;
  hash_output_indices_ = out.rvIndices();
  return out;
}

bool SymHash::canIgnore(size_t rv) {
  findIgnorableRVs();
  return ignorable_.count(rv) > 0;
}

void SymHash::findIgnorableRVs() {
  if (did_find_ignorable_) return;

  std::set<size_t> seen = {};
  ignorable_ = {};

  // At first we assume that ALL bits can be ignored
  for (size_t i : hash_input_indices_)
    ignorable_.insert(i);
  for (const auto &itr : Factor::global_factors)
    ignorable_.insert(itr.first);

  // Random variables in the queue cannot be ignored
  std::queue<size_t> q;
  for (size_t i : hash_output_indices_) q.push(i);

  while (!q.empty()) {
    const size_t rv = q.front();
    q.pop();
    ignorable_.erase(rv);
    seen.insert(rv);
    if (Factor::global_factors.count(rv) > 0) {
      const Factor &f = Factor::global_factors.at(rv);
      for (size_t inp : f.inputs) {
        if (seen.count(inp) == 0) q.push(inp);
      }
    }
  }

  did_find_ignorable_ = true;
}

}  // end namespace preimage