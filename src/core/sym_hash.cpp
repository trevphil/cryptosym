/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include "core/sym_hash.hpp"

#include "core/bit.hpp"
#include "core/config.hpp"
#include "core/logic_gate.hpp"
#include "core/utils.hpp"

namespace preimage {

SymHash::SymHash(int num_input_bits, int difficulty)
    : num_input_bits_(num_input_bits), difficulty_(difficulty) {
  if (num_input_bits_ % 8 != 0) {
    throw std::domain_error("Number of hash input bits should be a multiple of 8");
  }
}

SymHash::~SymHash() {}

int SymHash::numInputBits() const { return num_input_bits_; }

boost::dynamic_bitset<> SymHash::call(const boost::dynamic_bitset<> &hash_input) {
  const int inp_size = static_cast<int>(hash_input.size());
  if (inp_size != num_input_bits_) {
    char err_msg[128];
    snprintf(err_msg, 128, "Hash expected %d-bit input, got %d bits!", num_input_bits_,
             inp_size);
    throw std::length_error(err_msg);
  }

  SymBitVec inp(hash_input, false);
  SymBitVec out = hash(inp);
  return out.bits();
}

boost::dynamic_bitset<> SymHash::callRandom() {
  return call(utils::randomBits(num_input_bits_));
}

SymRepresentation SymHash::getSymbolicRepresentation() {
  Bit::reset();
  LogicGate::reset();
  SymBitVec inp(0, num_input_bits_, true);

  std::vector<int> input_indices(inp.size());
  for (int i = 0; i < inp.size(); i++) {
    input_indices[i] = inp.at(i).unknown ? inp.at(i).index : 0;
  }

  SymBitVec out = hash(inp);

  std::vector<int> output_indices(out.size());
  for (int i = 0; i < out.size(); i++) {
    output_indices[i] = out.at(i).unknown ? out.at(i).index : 0;
  }

  SymRepresentation sym_rep(LogicGate::global_gates, input_indices, output_indices);
  LogicGate::reset();
  Bit::reset();
  return sym_rep;
}

}  // end namespace preimage
