/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <cryptopp/dll.h>

#include <boost/dynamic_bitset.hpp>
#include <assert.h>

#include <map>
#include <string>
#include <vector>

#include "core/config.hpp"
#include "core/logic_gate.hpp"
#include "core/sym_bit_vec.hpp"
#include "core/utils.hpp"
#include "problem_instance.hpp"

namespace preimage {

void cmsatTests() {
  ProblemInstance problem(64, 12, false);

  int rtn = problem.prepare("SHA256", "cmsat");
  assert(rtn == 0);
  int status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("MD5", "cmsat");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("RIPEMD160", "cmsat");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  printf("%s\n", "CryptoMiniSAT tests passed.");
}

void preimageSATTests() {
  ProblemInstance problem(64, 12, false);

  int rtn = problem.prepare("SHA256", "simple");
  assert(rtn == 0);
  int status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("MD5", "simple");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("RIPEMD160", "simple");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  printf("%s\n", "PreimageSAT tests passed.");
}

void bpTests() {
  ProblemInstance problem(64, 2, false);

  int rtn = problem.prepare("SHA256", "bp");
  assert(rtn == 0);
  int status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("MD5", "bp");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  rtn = problem.prepare("RIPEMD160", "bp");
  assert(rtn == 0);
  status = problem.execute();
  assert(status == 0);

  printf("%s\n", "Belief propagation tests passed.");
}

}  // end namespace preimage

int main(int argc, char **argv) {
  printf("%s\n", "Running tests...");
  preimage::allTests();
  printf("%s\n", "All tests finished.");
  return 0;
}
