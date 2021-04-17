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

#include "hashing/sym_hash.hpp"
#include "bit.hpp"
#include "factor.hpp"
#include "utils.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>

namespace preimage {

SymHash::SymHash()
    : ignorable_(), did_find_ignorable_(false), hash_output_indices_() {}

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

void SymHash::saveFactorsCNF(const std::string &cnf_filename) {
  findIgnorableRVs();

  std::set<size_t> rvs;
  uint64_t n_clauses = 0;

  for (const Factor &f : Factor::global_factors) {
    if (ignorable_.count(f.output) > 0) continue;
    rvs.insert(f.output);
    for (size_t inp : f.inputs) rvs.insert(inp);
    if (f.t == Factor::Type::AndFactor)
      n_clauses += 3;
    else if (f.t == Factor::Type::NotFactor)
      n_clauses += 2;
    else if (f.t == Factor::Type::SameFactor)
      n_clauses += 2;
    else if (f.t == Factor::Type::XorFactor)
      n_clauses += 1;
    else if (f.t == Factor::Type::OrFactor)
      n_clauses += 3;
  }

  std::map<size_t, size_t> rv2idx;
  size_t idx = 1;
  for (size_t rv : rvs) rv2idx[rv] = idx++;

  std::ofstream cnf_file;
  cnf_file.open(cnf_filename);
  cnf_file << "p cnf " << rvs.size() << " " << n_clauses << std::endl;

  for (const Factor &f : Factor::global_factors) {
    if (ignorable_.count(f.output) > 0) continue;
    switch (f.t) {
      case Factor::Type::PriorFactor:
        break;
      case Factor::Type::NotFactor:
        cnf_file << rv2idx[f.inputs.at(0)] << " ";
        cnf_file << rv2idx[f.output] << " 0" << std::endl;
        cnf_file << "-" << rv2idx[f.inputs.at(0)] << " ";
        cnf_file << "-" << rv2idx[f.output] << " 0" << std::endl;
        break;
      case Factor::Type::SameFactor:
        cnf_file << rv2idx[f.inputs.at(0)] << " ";
        cnf_file << "-" << rv2idx[f.output] << " 0" << std::endl;
        cnf_file << rv2idx[f.output] << " ";
        cnf_file << "-" << rv2idx[f.inputs.at(0)] << " 0" << std::endl;
        break;
      case Factor::Type::AndFactor:
        cnf_file << rv2idx[f.inputs.at(0)] << " ";
        cnf_file << "-" << rv2idx[f.output] << " 0" << std::endl;
        cnf_file << rv2idx[f.inputs.at(1)] << " ";
        cnf_file << "-" << rv2idx[f.output] << " 0" << std::endl;
        cnf_file << rv2idx[f.output] << " ";
        cnf_file << "-" << rv2idx[f.inputs.at(0)] << " ";
        cnf_file << "-" << rv2idx[f.inputs.at(1)] << " 0" << std::endl;
        break;
      case Factor::Type::XorFactor:
        // If a = b ^ c, then a ^ b ^ c = 0 always
        // https://www.msoos.org/xor-clauses/
        cnf_file << "x" << rv2idx[f.output] << " ";
        cnf_file << "-" << rv2idx[f.inputs.at(0)] << " ";
        cnf_file << rv2idx[f.inputs.at(1)] << " 0" << std::endl;
        break;
      case Factor::Type::OrFactor:
        cnf_file << "-" << rv2idx[f.inputs.at(0)] << " ";
        cnf_file << rv2idx[f.output] << " 0" << std::endl;
        cnf_file << "-" << rv2idx[f.inputs.at(1)] << " ";
        cnf_file << rv2idx[f.output] << " 0" << std::endl;
        cnf_file << "-" << rv2idx[f.output] << " ";
        cnf_file << rv2idx[f.inputs.at(0)] << " ";
        cnf_file << rv2idx[f.inputs.at(1)] << " 0" << std::endl;
        break;
    }
  }

  cnf_file.close();
}

SymBitVec SymHash::hash(const SymBitVec &hash_input, int difficulty) {
  spdlog::error("Calling hash() from generic superclass");
  assert(false);
  return hash_input;
}

SymBitVec SymHash::call(const boost::dynamic_bitset<> &hash_input,
                        int difficulty) {
  Bit::reset();
  Factor::reset();
  did_find_ignorable_ = false;
  SymBitVec inp(hash_input, true);
  hash_input_indices_ = inp.rvIndices();
  SymBitVec out = hash(inp, difficulty);
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