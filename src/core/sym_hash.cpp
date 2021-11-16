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
#include "core/logic_gate.hpp"
#include "core/utils.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>
#include <queue>
#include <set>
#include <unordered_map>

#define SGN(x) (x < 0 ? -1 : (x > 0 ? 1 : 0))

namespace preimage {

SymHash::SymHash()
    : hash_input_indices_(), hash_output_indices_(),
      num_calls_(0.0), cum_runtime_ms_(0.0) {}

SymHash::~SymHash() {}

std::vector<int> SymHash::hashInputIndices() const {
  return hash_input_indices_;
}

std::vector<int> SymHash::hashOutputIndices() const {
  return hash_output_indices_;
}

double SymHash::averageRuntimeMs() const {
  if (num_calls_ == 0) return NAN;
  return cum_runtime_ms_ / num_calls_;
}

boost::dynamic_bitset<> SymHash::call(const boost::dynamic_bitset<> &hash_input,
                                      int difficulty, bool symbolic) {
  Bit::reset();
  LogicGate::reset();
  SymBitVec inp(hash_input, symbolic);
  hash_input_indices_ = std::vector<int>(inp.size());
  for (int i = 0; i < inp.size(); i++)
    hash_input_indices_[i] = inp.at(i).unknown ? inp.at(i).index : 0;
  if (difficulty == -1) difficulty = defaultDifficulty();
  const auto start = Utils::ms_since_epoch();
  SymBitVec out = hash(inp, difficulty);
  hash_output_indices_ = std::vector<int>(out.size());
  for (int i = 0; i < out.size(); i++)
    hash_output_indices_[i] = out.at(i).unknown ? out.at(i).index : 0;
  if (symbolic) pruneIrrelevantGates();
  if (symbolic) reindexBits();
  cum_runtime_ms_ += Utils::ms_since_epoch() - start;
  num_calls_++;
  return out.bits();
}

void SymHash::pruneIrrelevantGates() {
  const int n_before = static_cast<int>(LogicGate::global_gates.size());
  std::unordered_map<int, LogicGate> index2gate;
  for (const LogicGate &g : LogicGate::global_gates) {
    assert(g.output > 0);  // should always be positive
    index2gate[g.output] = g;
  }

  // Gates that appear in the queue are useful, not irrelevant
  std::queue<int> q;
  for (int i : hash_output_indices_) {
    if (i != 0) q.push(std::abs(i));
  }

  std::set<int> seen;  // Avoid re-visiting gates
  std::unordered_map<int, LogicGate> useful_gates;

  while (!q.empty()) {
    const int i = q.front();
    q.pop();
    if (seen.count(i) > 0) continue;
    seen.insert(i);

    if (index2gate.count(i) == 0) continue;
    const LogicGate &g = index2gate.at(i);
    useful_gates[i] = g;
    for (int inp : g.inputs) {
      if (seen.count(std::abs(inp)) == 0) {
        q.push(std::abs(inp));
      }
    }
  }

  LogicGate::global_gates = {};
  for (const auto &itr : useful_gates)
    LogicGate::global_gates.push_back(itr.second);

  const int n_after = static_cast<int>(LogicGate::global_gates.size());
  spdlog::info("Pruned gates ({} --> {}), removed {:.1f}%",
               n_before, n_after, 100.0 * (n_before - n_after) / n_before);
}

void SymHash::reindexBits() {
  // Get a set containing all bit indices which appear
  // in the reduced DAG (after removing irrelevant gates)
  std::set<int> old_indices;
  for (int out : hash_output_indices_) {
    if (out != 0) {
      old_indices.insert(std::abs(out));
    }
  }
  for (const LogicGate &g : LogicGate::global_gates) {
    old_indices.insert(std::abs(g.output));
    for (int inp : g.inputs) {
      old_indices.insert(std::abs(inp));
    }
  }

  // Index bits consecutively, starting at 1
  int k = 1;
  std::unordered_map<int, int> index_old2new;
  for (int old : old_indices) index_old2new[old] = k++;
  old_indices.clear();

  std::vector<LogicGate> new_gates;
  new_gates.reserve(LogicGate::global_gates.size());
  for (const LogicGate &g : LogicGate::global_gates) {
    std::vector<int> inputs;
    for (int inp : g.inputs) {
      inputs.push_back(SGN(inp) * index_old2new.at(std::abs(inp)));
    }
    const int output = SGN(g.output) * index_old2new.at(std::abs(g.output));
    LogicGate g_new(g.t(), g.depth, output, inputs);
    new_gates.push_back(g_new);
  }
  LogicGate::global_gates = new_gates;

  std::vector<int> new_inputs, new_outputs;
  for (int old_inp : hash_input_indices_) {
    if (index_old2new.count(std::abs(old_inp)) > 0) {
      new_inputs.push_back(SGN(old_inp) * index_old2new.at(std::abs(old_inp)));
    } else {
      new_inputs.push_back(0);
    }
  }
  for (int old_out : hash_output_indices_) {
    if (index_old2new.count(std::abs(old_out)) > 0) {
      new_outputs.push_back(SGN(old_out) * index_old2new.at(std::abs(old_out)));
    } else {
      new_outputs.push_back(0);
    }
  }
  hash_input_indices_ = new_inputs;
  hash_output_indices_ = new_outputs;
}

}  // end namespace preimage

#undef SGN
