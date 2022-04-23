/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <string>

#include "core/config.hpp"
#include "problem_instance.hpp"

namespace preimage {

std::string hash_func = "SHA256";
std::string solving_method = "simple";
int input_size = 64;
int difficulty = -1;
bool bin_format = false;

int parseArgument(char *arg) {
  int option = 0;
  unsigned int uoption = 0;
  char buf[1024] = "";

  if (strcmp(arg, "verbose") == 0) {
    config::verbose = true;
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
    help_msg << "\tverbose -> Include this argument for verbose output" << std::endl;
    help_msg << "\tbin   -> Include this argument to output bin instead of hex"
             << std::endl;
    help_msg << "\tand   -> Include this argument to only use AND logic gates"
             << std::endl;
    help_msg << "\thash=HASH_FUNCTION" << std::endl;
    help_msg << "\t -> one of: SHA256, MD5, RIPEMD160" << std::endl;
    help_msg << "\td=DIFFICULTY (-1 for default)" << std::endl;
    help_msg << "\ti=NUM_INPUT_BITS (choose a multiple of 8)" << std::endl;
    help_msg << "\tsolver=SOLVER" << std::endl;
    help_msg << "\t -> one of: cmsat, simple, bp" << std::endl;
    printf("%s\n", help_msg.str().c_str());
    return 1;
  }

  return 0;
}

void run(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (parseArgument(argv[i]) != 0) return;
  }

  ProblemInstance problem(input_size, difficulty, bin_format);
  const int rtn = problem.prepare(hash_func, solving_method);

  if (rtn != 0) {
    printf("%s\n", "Error configuring the problem, exiting.");
    return;
  }

  const int status = problem.execute();
  if (status == 0) {
    printf("%s\n", "Problem was solved!");
  } else {
    printf("Problem was not solved! (status=%d)\n", status);
  }

  printf("%s\n", "Done.");
}

}  // end namespace preimage

int main(int argc, char **argv) {
  preimage::run(argc, argv);
  return 0;
}
