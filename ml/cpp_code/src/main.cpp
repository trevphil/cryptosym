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
#include "spdlog/sinks/basic_file_sink.h"

#include "utils/config.hpp"
#include "hash_reversal/undirected_graph.hpp"

int main(int argc, char** argv) {
	if (argc < 2) {
		spdlog::error("You must provide a path to a YAML config file!");
		return -1;
	}

	std::string config_file = argv[1];
	utils::Config config = utils::Config(config_file);

	hash_reversal::UndirectedGraph udg;

	return 0;
}