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

#include "factor.hpp"

#include <iomanip>
#include <sstream>

namespace dataset_generator {

std::vector<Factor> Factor::global_factors = {};

Factor::Factor() : valid(false) {}

Factor::Factor(Factor::Type typ, const size_t out,
               const std::vector<size_t> &inp)
    : t(typ), output(out), inputs(inp), valid(true) {
  n_inputs = Factor::numInputs(t);
  if (n_inputs != inp.size()) {
    spdlog::info("Factor {} requires {} input(s) but got {}", char(t), n_inputs,
                 inp.size());
  }
  assert(n_inputs == inp.size());
}

Factor::~Factor() {}

std::string Factor::toString() const {
  std::stringstream stream;
  stream << char(t) << " " << output;
  for (size_t inp : inputs) stream << " " << inp;
  std::string result(stream.str());
  return result;
}

void Factor::reset() { global_factors = {}; }

size_t Factor::numInputs(Factor::Type t) {
  switch (t) {
    case Type::PriorFactor:
      return 0;
    case Type::NotFactor:
      return 1;
    case Type::SameFactor:
      return 1;
    case Type::AndFactor:
      return 2;
    case Type::XorFactor:
      return 2;
  }
}

}  // end namespace dataset_generator