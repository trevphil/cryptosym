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

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <boost/dynamic_bitset.hpp>
#include <cryptominisat5/cryptominisat.h>

#include <set>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <unordered_map>

#include "core/utils.hpp"
#include "core/logic_gate.hpp"
#include "core/sym_hash.hpp"
#include "hashing/hash_funcs.hpp"

#define N_TRAIN 40000
#define N_VAL 1000
#define N_TEST 1000
#define MIN_INPUT_SIZE 64
#define MAX_INPUT_SIZE 128
#define INPUT_SIZE_RANGE (MAX_INPUT_SIZE - MIN_INPUT_SIZE)
#define MAX_DIFFICULTY 8

namespace preimage {

void saveSample(const std::string &split_name,
                const int sample_idx,
                const std::vector<std::set<int>> &cnf,
                const std::unordered_map<int, bool> &solution) {
}

int simplifyCNF(std::vector<std::set<int>> &cnf,
                const std::unordered_map<int, bool> &observed) {
  std::vector<std::pair<int, bool>> q;
  std::unordered_map<int, bool> assignments = observed;
  std::unordered_map<int, std::set<int>> lit2clauses;

  for (int c = 0; c < static_cast<int>(cnf.size()); c++) {
    for (int lit : cnf[c]) lit2clauses[lit].insert(c);
  }

  for (const auto &itr : assignments) {
    assert(itr.first != 0);
    q.push_back({itr.first, itr.second});
    q.push_back({-itr.first, !itr.second});
  }

  while (q.size() > 0) {
    const int lit = q.back().first;
    const bool val = q.back().second;
    q.pop_back();
    const std::set<int> &clause_indices = lit2clauses[lit];
    if (val) {
      // All referenced clauses are automatically SAT
      for (int clause_idx : clause_indices) cnf[clause_idx] = {};
    } else {
      for (int clause_idx : clause_indices) {
        if (cnf[clause_idx].size() == 0) continue;
        // If this is the last literal in the clause, UNSAT
        assert(cnf[clause_idx].size() != 1);
        // Remove literal from clause, since it is 0 / false
        cnf[clause_idx].erase(lit);
        if (cnf[clause_idx].size() == 1) {
          const int last_lit = *(cnf[clause_idx].begin());
          if (assignments.count(last_lit) && !assignments[last_lit]) {
            assert(false);  // UNSAT because we need last_lit = 1
          }
          if (assignments.count(-last_lit) && assignments[-last_lit]) {
            assert(false);  // UNSAT, need -last_lit = 0 --> last_lit = 1
          }
          q.push_back({last_lit, true});
          q.push_back({-last_lit, false});
          assignments[last_lit] = true;
          assignments[-last_lit] = false;
          cnf[clause_idx] = {};
        }
      }
    }
  }

  std::unordered_map<int, int> var_remapping;
  std::vector<std::set<int>> new_cnf;
  int k = 1;

  for (const std::set<int> &old_clause : cnf) {
    if (old_clause.size() == 0) continue;
    std::set<int> new_clause;
    for (int old_lit : old_clause) {
      if (var_remapping.count(old_lit)) {
        new_clause.insert(var_remapping[old_lit]);
      } else {
        var_remapping[std::abs(old_lit)] = k;
        var_remapping[-std::abs(old_lit)] = -k;
        new_clause.insert(var_remapping[old_lit]);
        k++;
      }
    }
    new_cnf.push_back(new_clause);
  }

  cnf = new_cnf;
  return k - 1;
}

int randInputSize() {
  int inp_size = 0;
  while (inp_size < MIN_INPUT_SIZE) {
    inp_size = MIN_INPUT_SIZE + (rand() % INPUT_SIZE_RANGE);
    inp_size -= (inp_size % 8);
  }
  assert(inp_size % 8 == 0);
  return inp_size;
}

void clause2graph(const std::set<int> &clause_set,
                  const std::unordered_map<int, int> &lit2node_index,
                  int &current_node_index,
                  std::vector<std::pair<int, int>> &edges) {
  assert(clause_set.size() > 1);
  const std::vector<int> clause(clause_set.begin(), clause_set.end());
  int node1 = lit2node_index.at(clause[0]);
  for (int i = 1; i < static_cast<int>(clause.size()); ++i) {
    const int node2 = lit2node_index.at(clause[i]);
    edges.push_back({node1, current_node_index});
    edges.push_back({node2, current_node_index + 1});
    edges.push_back({current_node_index, current_node_index + 1});
    edges.push_back({current_node_index, current_node_index + 2});
    edges.push_back({current_node_index + 1, current_node_index + 2});
    node1 = current_node_index + 2;
    current_node_index += 3;
  }
  // Force node1 (OR of literals) to be true by connecting to B and F
  edges.push_back({node1, 0});
  edges.push_back({node1, 2});
}

int takeSample(std::shared_ptr<SymHash> hasher) {
  // Pick random input and difficulty
  const int inp_size = randInputSize();
  const int difficulty = 1 + (rand() % MAX_DIFFICULTY);
  spdlog::info("\t|input|={}, difficulty={}", inp_size, difficulty);
  boost::dynamic_bitset<> input = Utils::randomBits(inp_size);
  boost::dynamic_bitset<> output = hasher->call(input, difficulty, true);

  // Convert to CNF
  std::vector<std::set<int>> cnf;
  for (const LogicGate &g : LogicGate::global_gates) {
    const std::vector<std::vector<int>> clauses = g.cnf();
    for (const std::vector<int> &c : clauses) {
      cnf.push_back(std::set<int>(c.begin(), c.end()));
    }
  }

  // Get observed bits
  std::unordered_map<int, bool> assignments;
  const std::vector<int> out_idx = hasher->hashOutputIndices();
  for (uint32_t j = 0; j < out_idx.size(); ++j) {
    if (out_idx[j] == 0) continue;
    assignments[out_idx[j]] = output[j];
  }

  // Simplify CNF (remove all clauses with 1 lit), and re-index
  spdlog::info("\t# clauses before simplify: {}", cnf.size());
  const int num_vars = simplifyCNF(cnf, assignments);
  spdlog::info("\t# clauses after simplify: {}", cnf.size());
  spdlog::info("\tnum_vars={}", num_vars);

  // Coloring: 0 = False, 1 = True, 2 = Base
  std::vector<std::pair<int, int>> edges = {
    std::make_pair<int, int>(0, 1),
    std::make_pair<int, int>(0, 2),
    std::make_pair<int, int>(1, 2)
  };
  int current_node_index = 3;
  std::unordered_map<int, int> lit2node_index;
  for (int var = 1; var <= num_vars; ++var) {
    lit2node_index[var] = current_node_index;
    lit2node_index[-var] = current_node_index + 1;
    current_node_index += 2;
  }
  for (const std::set<int> &clause : cnf) {
    clause2graph(clause, lit2node_index, current_node_index, edges);
  }
  spdlog::info("\t# edges: {}", edges.size());
  return 1;
}

void generateSplit(const std::string &split_name,
                   const int num_samples,
                   std::shared_ptr<SymHash> hasher) {
  spdlog::info("Generating split {} (N={})", split_name, num_samples);

  int sample_idx = 0;
  while (sample_idx < num_samples) {
    spdlog::info("Sample {}", sample_idx);
    if (takeSample(hasher)) sample_idx++;
  }
}

void generateAll() {
  std::shared_ptr<SymHash> train_hasher = std::shared_ptr<SymHash>(new SHA256());
  std::shared_ptr<SymHash> val_hasher = std::shared_ptr<SymHash>(new RIPEMD160());
  std::shared_ptr<SymHash> test_hasher = std::shared_ptr<SymHash>(new SHA256());

  generateSplit("train", N_TRAIN, train_hasher);
  generateSplit("val", N_VAL, val_hasher);
  generateSplit("test", N_TEST, test_hasher);
}

}  // end namespace preimage

int main(int argc, char **argv) {
  preimage::generateAll();
  return 0;
}
