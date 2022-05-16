
/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include "eval_solver.hpp"

#include <unordered_map>

#include "core/bit_vec.hpp"
#include "core/sym_representation.hpp"

namespace preimage {

bool evaluateSolver(std::shared_ptr<Solver> solver, std::shared_ptr<SymHash> hasher) {
  const BitVec expected_hash = hasher->callRandom();
  const SymRepresentation problem = hasher->getSymbolicRepresentation();
  const std::unordered_map<int, bool> solution = solver->solve(problem, expected_hash);

  BitVec preimage(hasher->numInputBits());
  for (int k = 0; k < hasher->numInputBits(); k++) {
    const int input_index = problem.inputIndices().at(k);
    if (input_index < 0 && solution.count(-input_index) > 0) {
      preimage[k] = !solution.at(-input_index);
    } else if (input_index > 0 && solution.count(input_index) > 0) {
      preimage[k] = solution.at(input_index);
    } else {
      preimage[k] = 0;
    }
  }

  const BitVec actual_hash = hasher->call(preimage);
  return expected_hash == actual_hash;
}

}  // end namespace preimage
