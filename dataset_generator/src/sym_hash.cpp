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

#include "sym_hash.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>
#include <map>

#include "bit.hpp"
#include "factor.hpp"
#include "utils.hpp"

namespace dataset_generator {

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

void SymHash::saveFactors(const std::string &factor_filename,
                          const std::string &cnf_filename) {
  findIgnorableRVs();

  std::ofstream factor_file;
  factor_file.open(factor_filename);
  std::set<size_t> rvs;
  uint64_t n_clauses = 0;

  for (const Factor &f : Factor::global_factors) {
    if (ignorable_.count(f.output) > 0) continue;
    factor_file << f.toString() << "\n";
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
  }
  factor_file.close();

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
  for (size_t i = 0; i < num_bits; ++i) ignorable_.insert(i);

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

}  // end namespace dataset_generator