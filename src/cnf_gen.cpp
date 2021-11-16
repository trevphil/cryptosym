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

#include "core/cnf.hpp"
#include "core/utils.hpp"
#include "core/logic_gate.hpp"
#include "core/sym_hash.hpp"
#include "hashing/hash_funcs.hpp"

#define MIN_INPUT_SIZE 64
#define MAX_INPUT_SIZE 128
#define INPUT_SIZE_RANGE (MAX_INPUT_SIZE - MIN_INPUT_SIZE + 1)
#define MIN_DIFFICULTY 17
#define MAX_DIFFICULTY 32
#define DIFFICULTY_RANGE (MAX_DIFFICULTY - MIN_DIFFICULTY + 1)

namespace preimage {

int randInputSize() {
  int inp_size = 0;
  while (inp_size < MIN_INPUT_SIZE) {
    inp_size = MIN_INPUT_SIZE + (rand() % INPUT_SIZE_RANGE);
    inp_size -= (inp_size % 8);
  }
  assert(inp_size % 8 == 0);
  return inp_size;
}

int randDifficulty() {
  return MIN_DIFFICULTY + (rand() % DIFFICULTY_RANGE);
}

CNF takeSample(std::shared_ptr<SymHash> hasher) {
  // Pick random input and difficulty
  const int inp_size = randInputSize();
  const int difficulty = randDifficulty();
  spdlog::info("\t|input|={}, difficulty={}", inp_size, difficulty);
  boost::dynamic_bitset<> input = Utils::randomBits(inp_size);
  boost::dynamic_bitset<> output = hasher->call(input, difficulty, true);

  // Get observed bits
  std::unordered_map<int, bool> assignments;
  const std::vector<int> out_idx = hasher->hashOutputIndices();
  for (size_t j = 0; j < out_idx.size(); ++j) {
    if (out_idx[j] == 0) continue;
    assignments[out_idx[j]] = output[j];
  }

  return CNF(LogicGate::global_gates).simplify(assignments);
}

void generate(int n) {
  std::vector<std::shared_ptr<SymHash>> hash_funcs = {
    std::shared_ptr<SymHash>(new SHA256()),
    std::shared_ptr<SymHash>(new RIPEMD160()),
    std::shared_ptr<SymHash>(new SHA256())
  };

  for (int i = 0; i < n; ++i) {
    const auto hasher = hash_funcs[rand() % hash_funcs.size()];
    const CNF cnf = takeSample(hasher);
    spdlog::info("Sample {} - {}", i + 1, hasher->hashName());
    spdlog::info("\t# vars={}", cnf.num_vars);
    spdlog::info("\t# clauses: {}", cnf.num_clauses);
    char fname[16];
    sprintf(fname, "cnf/%04d.cnf", i);
    cnf.write(std::string(fname));
  }
}

}  // end namespace preimage

int main(int argc, char **argv) {
  preimage::generate(50);
  return 0;
}
