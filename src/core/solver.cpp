/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include "core/solver.hpp"

#include <algorithm>

#include "core/config.hpp"
#include "core/utils.hpp"

namespace preimage {

Solver::Solver() {}

Solver::~Solver() {}

std::unordered_map<int, bool> Solver::solve(const SymRepresentation &problem,
                                            const std::string &hash_hex) {
  const BitVec bitvec = utils::hex2bits(hash_hex);
  return solve(problem, bitvec);
}

std::unordered_map<int, bool> Solver::solve(const SymRepresentation &problem,
                                            const BitVec &hash_output) {
  const std::vector<int> &output_indices = problem.outputIndices();
  const unsigned int output_size = output_indices.size();
  std::unordered_map<int, bool> assignments;
  for (unsigned int k = 0; k < output_size; k++) {
    // If `hash_output` converted from hex, zeros in MSBs could shorten bitvec
    const bool bitval = k < hash_output.size() ? hash_output[k] : false;
    const int output_index = output_indices.at(k);
    if (output_index < 0) {
      assignments[-output_index] = !bitval;
    } else if (output_index > 0) {
      assignments[output_index] = bitval;
    }
  }
  return solve(problem, assignments);
}

}  // end namespace preimage
