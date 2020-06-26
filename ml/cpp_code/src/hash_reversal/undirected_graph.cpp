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

#include "spdlog/spdlog.h"

#include "hash_reversal/undirected_graph.hpp"

namespace hash_reversal {

UndirectedGraph::UndirectedGraph() {
	spdlog::info("Undirected graph was initialized!");
}

}  // end namespace hash_reversal