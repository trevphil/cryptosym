/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>

#include "core/bit.hpp"
#include "core/config.hpp"
#include "core/logic_gate.hpp"
#include "core/sym_hash.hpp"
#include "core/utils.hpp"

#define SGN(x) (x < 0 ? -1 : (x > 0 ? 1 : 0))

namespace preimage {

SymRepresentation::SymRepresentation(const std::vector<int> &input_indices,
                                     const std::vector<int> &output_indices)
    : hash_input_indices_(input_indices), hash_output_indices_(output_indices) {
  gates_ = LogicGate::global_gates;
  pruneIrrelevantGates();
  reindexBits();
}

int SymRepresentation::numVars() const { return num_vars_; }

std::vector<LogicGate> SymRepresentation::gates() const { return gates_; }

std::vector<int> SymRepresentation::hashInputIndices() const {
  return hash_input_indices_;
}

std::vector<int> SymRepresentation::hashOutputIndices() const {
  return hash_output_indices_;
}

void SymRepresentation::toDAG(const std::string &filename) const {
  throw std::logic_error("Function not yet implemented.");
}

void SymRepresentation::toCNF(const std::string &filename) const {
  throw std::logic_error("Function not yet implemented.");
}

void SymRepresentation::toMIP(const std::string &filename) const {
  throw std::logic_error("Function not yet implemented.");
}

void SymRepresentation::toGraphColoring(const std::string &filename) const {
  throw std::logic_error("Function not yet implemented.");
}

void SymRepresentation::pruneIrrelevantGates() {
  const int n_before = static_cast<int>(gates_.size());
  std::unordered_map<int, LogicGate> index2gate;
  for (const LogicGate &g : gates_) {
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

  gates_.clear();
  gates_.reserve(useful_gates.size());
  for (const auto &itr : useful_gates) gates_.push_back(itr.second);

  if (config::verbose) {
    const int n_after = static_cast<int>(gates_.size());
    printf("Pruned gates (%d --> %d), removed %.1f%%\n", n_before, n_after,
           100.0 * (n_before - n_after) / n_before);
  }
}

void SymRepresentation::reindexBits() {
  // Get a set containing all bit indices which appear
  // in the reduced DAG (after removing irrelevant gates)
  std::set<int> old_indices;
  for (int out : hash_output_indices_) {
    if (out != 0) {
      old_indices.insert(std::abs(out));
    }
  }
  for (const LogicGate &g : gates_) {
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
  new_gates.reserve(gates_.size());
  for (const LogicGate &g : gates_) {
    std::vector<int> inputs;
    for (int inp : g.inputs) {
      inputs.push_back(SGN(inp) * index_old2new.at(std::abs(inp)));
    }
    const int output = SGN(g.output) * index_old2new.at(std::abs(g.output));
    LogicGate g_new(g.t(), g.depth, output, inputs);
    new_gates.push_back(g_new);
  }

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

  num_vars_ = k - 1;
  gates_ = new_gates;
  hash_input_indices_ = new_inputs;
  hash_output_indices_ = new_outputs;
}

}  // end namespace preimage
