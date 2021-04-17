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

double SymHash::averageRuntimeMs() const {
  if (num_calls_ == 0) return -1.0;
  return cum_runtime_ms_ / num_calls_;
}

void SymHash::saveStatistics(const std::string &stats_filename) {
  findIgnorableRVs();
  const size_t n = Factor::global_factors.size();
  std::map<Factor::Type, size_t> factor_count;
  std::map<size_t, size_t> and_gap_count;
  std::map<size_t, size_t> xor_gap_count;
  std::map<size_t, size_t> or_gap_count;
  for (size_t i = 0; i < n; ++i) {
    const Factor &f = Factor::global_factors.at(i);
    assert(i == f.output);
    if (canIgnore(i)) continue;
    factor_count[f.t]++;
    if (f.t == Factor::Type::AndFactor || f.t == Factor::Type::XorFactor ||
        f.t == Factor::Type::OrFactor) {
      const size_t inp1 = std::min(f.inputs.at(0), f.inputs.at(1));
      const size_t inp2 = std::max(f.inputs.at(0), f.inputs.at(1));
      const size_t gap = inp2 - inp1;
      if (f.t == Factor::Type::AndFactor) {
        and_gap_count[gap]++;
      } else if (f.t == Factor::Type::XorFactor) {
        xor_gap_count[gap]++;
      } else if (f.t == Factor::Type::OrFactor) {
        or_gap_count[gap]++;
      }
    }
  }

  std::ofstream stats_file;
  stats_file.open(stats_filename);
  stats_file << "factors " << factor_count.size() << std::endl;
  for (const auto &itr : factor_count) {
    const Factor::Type t = itr.first;
    const size_t cnt = itr.second;
    switch (t) {
    case Factor::Type::PriorFactor:
      stats_file << "P " << cnt << std::endl;
      break;
    case Factor::Type::SameFactor:
      stats_file << "S " << cnt << std::endl;
      break;
    case Factor::Type::NotFactor:
      stats_file << "N " << cnt << std::endl;
      break;
    case Factor::Type::AndFactor:
      stats_file << "A " << cnt << std::endl;
      break;
    case Factor::Type::XorFactor:
      stats_file << "X " << cnt << std::endl;
      break;
    case Factor::Type::OrFactor:
      stats_file << "O " << cnt << std::endl;
    }
  }

  stats_file << "and_gap_count " << and_gap_count.size() << std::endl;
  for (const auto &itr : and_gap_count) {
    const size_t gap = itr.first;
    const size_t cnt = itr.second;
    stats_file << gap << " " << cnt << std::endl;
  }

  stats_file << "xor_gap_count " << xor_gap_count.size() << std::endl;
  for (const auto &itr : xor_gap_count) {
    const size_t gap = itr.first;
    const size_t cnt = itr.second;
    stats_file << gap << " " << cnt << std::endl;
  }

  stats_file << "or_gap_count " << or_gap_count.size() << std::endl;
  for (const auto &itr : or_gap_count) {
    const size_t gap = itr.first;
    const size_t cnt = itr.second;
    stats_file << gap << " " << cnt << std::endl;
  }

  stats_file.close();
}

void SymHash::saveFactors(const std::string &factor_filename) {
  findIgnorableRVs();
  std::ofstream factor_file;
  factor_file.open(factor_filename);
  for (const Factor &f : Factor::global_factors) {
    if (ignorable_.count(f.output) > 0) continue;
    factor_file << f.toString() << "\n";
  }
  factor_file.close();
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
  assert(Factor::global_factors.size() == Bit::global_bits.size());
  hash_output_indices_ = out.rvIndices();
  return out;
}

bool SymHash::canIgnore(size_t rv) {
  findIgnorableRVs();
  return ignorable_.count(rv) > 0;
}

void SymHash::findIgnorableRVs() {
  if (did_find_ignorable_) return;

  size_t idx = 0;
  size_t num_bits = numUnknownsPerHash();
  std::set<size_t> seen = {};
  ignorable_ = {};
  // At first we assume that ALL bits can be ignored
  for (size_t i = 0; i < num_bits; ++i) ignorable_.insert(i);

  // Random variables in the queue cannot be ignored
  std::vector<size_t> queue;
  for (size_t i : hash_output_indices_) queue.push_back(i);

  while (idx < queue.size()) {
    const size_t rv = queue.at(idx);  // do not pop from queue, to speed it up
    ignorable_.erase(rv);
    seen.insert(rv);
    const Factor &f = Factor::global_factors.at(rv);
    for (size_t inp : f.inputs) {
      if (seen.count(inp) == 0) queue.push_back(inp);
    }
    ++idx;
  }

  did_find_ignorable_ = true;
}

}  // end namespace preimage