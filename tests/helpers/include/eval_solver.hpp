
/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <memory>

#include "core/solver.hpp"
#include "core/sym_hash.hpp"

namespace preimage {

bool evaluateSolver(std::shared_ptr<Solver> solver, std::shared_ptr<SymHash> hasher);

}  // end namespace preimage
