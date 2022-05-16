/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include "bp/bp_solver.hpp"
#include "cmsat/cmsat_solver.hpp"
#include "core/config.hpp"
#include "core/logic_gate.hpp"
#include "core/solver.hpp"
#include "core/sym_hash.hpp"
#include "core/sym_representation.hpp"
#include "core/utils.hpp"
#include "dag_solver/dag_solver.hpp"
#include "hashing/sym_md5.hpp"
#include "hashing/sym_ripemd160.hpp"
#include "hashing/sym_sha256.hpp"

namespace preimage {

std::string hash_func = "SHA256";
std::string solving_method = "dag";
int input_size = 64;
int difficulty = -1;
bool bin_format = false;

int parseArgument(char *arg) {
  int option = 0;
  unsigned int uoption = 0;
  char buf[1024] = "";

  if (strcmp(arg, "quiet") == 0) {
    config::verbose = false;
  } else if (strcmp(arg, "bin") == 0) {
    bin_format = true;
  } else if (strcmp(arg, "and") == 0) {
    config::only_and_gates = true;
  } else if (1 == sscanf(arg, "hash=%s", buf)) {
    hash_func = buf;
  } else if (1 == sscanf(arg, "d=%d", &option)) {
    difficulty = option;
  } else if (1 == sscanf(arg, "i=%u", &uoption)) {
    input_size = int(uoption);
  } else if (1 == sscanf(arg, "solver=%s", buf)) {
    solving_method = buf;
  } else {
    std::stringstream help_msg;
    help_msg << std::endl << "Command-line arguments:" << std::endl;
    help_msg << "\tquiet -> Disable verbose output" << std::endl;
    help_msg << "\tbin   -> Output binary instead of hex" << std::endl;
    help_msg << "\tand   -> Reduce all logic gates to AND and NOT gates" << std::endl;
    help_msg << "\thash=HASH_FUNCTION" << std::endl;
    help_msg << "\t -> one of: SHA256, MD5, RIPEMD160" << std::endl;
    help_msg << "\td=DIFFICULTY (-1 for hash's default difficulty)" << std::endl;
    help_msg << "\ti=NUM_INPUT_BITS (must be a multiple of 8)" << std::endl;
    help_msg << "\tsolver=SOLVER" << std::endl;
    help_msg << "\t -> one of: dag, cmsat, bp" << std::endl;
    printf("%s\n", help_msg.str().c_str());
    return 1;
  }

  return 0;
}

std::unique_ptr<SymHash> createHashFunction() {
  if (hash_func.compare("SHA256") == 0) {
    return std::make_unique<SHA256>(input_size, difficulty);
  } else if (hash_func.compare("MD5") == 0) {
    return std::make_unique<MD5>(input_size, difficulty);
  } else if (hash_func.compare("RIPEMD160") == 0) {
    return std::make_unique<RIPEMD160>(input_size, difficulty);
  } else {
    char err_msg[128];
    snprintf(err_msg, 128, "Unsupported hash function: %s", hash_func.c_str());
    throw std::runtime_error(err_msg);
  }
}

std::unique_ptr<Solver> createSolver() {
  if (solving_method.compare("cmsat") == 0) {
    return std::make_unique<CMSatSolver>();
  } else if (solving_method.compare("dag") == 0) {
    return std::make_unique<DAGSolver>();
  } else if (solving_method.compare("bp") == 0) {
    return std::make_unique<bp::BPSolver>();
  } else {
    char err_msg[128];
    snprintf(err_msg, 128, "Unsupported solver: %s", solving_method.c_str());
    throw std::runtime_error(err_msg);
  }
}

void run(int argc, char **argv) {
  config::verbose = true;
  for (int i = 1; i < argc; i++) {
    if (parseArgument(argv[i]) != 0) return;
  }

  std::unique_ptr<SymHash> hasher = createHashFunction();
  std::unique_ptr<Solver> solver = createSolver();

  // Print basic information about the problem
  printf("Hash algorithm:\t%s\n", hasher->hashName().c_str());
  printf("Solver:\t\t%s\n", solver->solverName().c_str());
  printf("Input message size:\t%d bits\n", input_size);
  printf("Difficulty level:\t%d\n", difficulty);
  printf("%s\n", "-----------------------");

  // Print problem size and logic gate distribution
  const SymRepresentation problem = hasher->getSymbolicRepresentation();
  std::map<LogicGate::Type, int> gate_counts;
  for (const LogicGate &g : problem.gates()) gate_counts[g.t()]++;
  const double c = 100.0 / static_cast<double>(problem.gates().size());
  printf("Number of variables: %d\n", problem.numVars());
  printf("%s\n", "Logic gate distribution:");
  for (const auto &itr : gate_counts) {
    const std::string gate_str = LogicGate::humanReadableType(itr.first);
    printf("\t%s:\t%d\t(%.1f%%)\n", gate_str.c_str(), itr.second, itr.second * c);
  }
  printf("%s\n", "-----------------------");

  // Solve the problem
  const boost::dynamic_bitset<> true_input = utils::randomBits(input_size);
  const boost::dynamic_bitset<> true_hash = hasher->call(true_input);
  const std::unordered_map<int, bool> solution = solver->solve(problem, true_hash);

  // Build input bits from the solution
  boost::dynamic_bitset<> preimage(input_size);
  for (int k = 0; k < input_size; k++) {
    const int input_index = problem.inputIndices().at(k);
    if (input_index < 0 && solution.count(-input_index) > 0) {
      preimage[k] = !solution.at(-input_index);
    } else if (input_index > 0 && solution.count(input_index) > 0) {
      preimage[k] = solution.at(input_index);
    } else {
      preimage[k] = 0;
    }
  }
  const boost::dynamic_bitset<> actual_hash = hasher->call(preimage);

  // Compare the input to the hash function with the reconstructed input
  if (bin_format) {
    printf("True input:\t\t%s\n", utils::binstr(true_input).c_str());
    printf("Reconstructed input:\t%s\n", utils::binstr(preimage).c_str());
  } else {
    printf("True input:\t\t%s\n", utils::hexstr(true_input).c_str());
    printf("Reconstructed input:\t%s\n", utils::hexstr(preimage).c_str());
  }

  // Check if the reconstructed input yields the expected hash
  const std::string expected =
      bin_format ? utils::binstr(true_hash) : utils::hexstr(true_hash);
  const std::string actual =
      bin_format ? utils::binstr(actual_hash) : utils::hexstr(actual_hash);
  if (expected.compare(actual) == 0) {
    printf("Success! Hashes match:\t%s\n", expected.c_str());
  } else {
    printf("%s\n", "!!! Hashes do not match.");
    printf("\tExpected:\t%s\n", expected.c_str());
    printf("\tGot:\t\t%s\n", actual.c_str());
  }

  printf("%s\n", "Done.");
}

}  // end namespace preimage

int main(int argc, char **argv) {
  preimage::run(argc, argv);
  return 0;
}
