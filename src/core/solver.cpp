/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
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
  const boost::dynamic_bitset<> bitvec = utils::hex2bits(hash_hex);
  return solve(problem, bitvec);
}

std::unordered_map<int, bool> Solver::solve(const SymRepresentation &problem,
                                            const boost::dynamic_bitset<> &hash_output) {
  const std::vector<int> &output_indices = problem.hashOutputIndices();
  const int output_size = static_cast<int>(output_indices.size());
  std::unordered_map<int, bool> assignments;
  for (int k = 0; k < output_size; k++) {
    // If `hash_output` converted from hex, zeros in MSBs could shorten bitset
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
