/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include "preimage_sat/preimage_sat.hpp"

#include <iostream>
#include <queue>

#include "core/config.hpp"
#include "core/utils.hpp"

namespace preimage {

PreimageSATSolver::PreimageSATSolver() : Solver() {}

PreimageSATSolver::~PreimageSATSolver() {}

void PreimageSATSolver::initialize() {
  const auto start = Utils::ms_since_epoch();

  // At first, all literals are unassigned (value = 0)
  literals = std::vector<int8_t>(num_vars_ + 1, 0);
  stack.clear();  // Stack of literal assignments is empty

  // Mapping from literal index to a list of gates using that literal
  lit2gates.clear();
  for (int i = 0; i < static_cast<int>(gates_.size()); ++i) {
    const LogicGate &g = gates_.at(i);
    const int out = g.output;
    assert(out > 0);
    lit2gates[out].insert(i);
    for (const int inp : g.inputs) {
      assert(inp != 0);
      lit2gates[std::abs(inp)].insert(i);
    }
  }

  // Order literals from most --> least useful
  literal_ordering = std::vector<LitStats>(num_vars_);
  for (int i = 1; i <= num_vars_; i++) {
    literal_ordering[i - 1] = computeStats(i);
  }
  std::sort(literal_ordering.begin(), literal_ordering.end(),
            [](const LitStats &a, const LitStats &b) { return a.score() > b.score(); });

  const double init_time_ms = Utils::ms_since_epoch() - start;
  if (config::verbose) printf("Initialized PreimageSAT in %.0f ms\n", init_time_ms);
}

std::unordered_map<int, bool> PreimageSATSolver::solveInternal() {
  for (const auto &itr : observed_) {
    const int lit = itr.first;
    assert(lit > 0);
    const bool truth_value = itr.second;
    if (literals[lit] == 0) {
      pushStack(lit, truth_value, false);
      const int num_solved = propagate(lit);
      if (num_solved < 0) {
        printf("%s\n", "Problem is UNSAT!");
        return {};
      }
    } else if (truth_value != (literals[lit] > 0)) {
      printf("%s\n", "Problem is UNSAT!");
      return {};
    }
  }

  bool preferred_assignment;
  int picked_lit = pickLiteral(preferred_assignment);
  pushStack(picked_lit, preferred_assignment, false);
  bool conflict = propagate(picked_lit) < 0;
  bool success = false;

  while (picked_lit > 0) {
    if (conflict) {
      while (stack.back().second_try) {
        success = popStack();
        assert(success);
      }
      success = popStack(picked_lit, preferred_assignment);
      assert(success);
      pushStack(picked_lit, !preferred_assignment, true);
      conflict = propagate(picked_lit) < 0;
    } else {
      picked_lit = pickLiteral(preferred_assignment);
      pushStack(picked_lit, preferred_assignment, false);
      conflict = propagate(picked_lit) < 0;
    }
  }

  std::unordered_map<int, bool> solution;
  for (int i = 1; i <= num_vars_; ++i) {
    if (literals[i] != 0) solution[i] = (literals[i] > 0);
  }
  return solution;
}

PreimageSATSolver::LitStats PreimageSATSolver::computeStats(const int lit) {
  LitStats stats(lit, false);
  stats.num_referenced_gates = lit2gates[lit].size();
  return stats;
}

void PreimageSATSolver::pushStack(int lit, bool truth_value, bool second_try) {
  assert(literals[lit] == 0);
  literals[lit] = truth_value ? 1 : -1;
  stack.push_back(StackItem(lit));
  stack.back().second_try = second_try;
}

bool PreimageSATSolver::popStack(int &lit, bool &truth_value) {
  if (stack.size() <= observed_.size()) return false;
  const auto &back = stack.back();
  lit = back.lit_guess;
  truth_value = (literals[lit] > 0);
  literals[lit] = 0;
  for (int implied : back.implied) literals[implied] = 0;
  stack.pop_back();
  return true;
}

bool PreimageSATSolver::popStack() {
  int lit;
  bool truth_value;
  return popStack(lit, truth_value);
}

int PreimageSATSolver::pickLiteral(bool &assignment) {
  int chosen_lit = 0;
  for (int i = 0; i < num_vars_; ++i) {
    const int lit = literal_ordering.at(i).lit;
    if (literals[lit] == 0) {
      chosen_lit = lit;
      assignment = literal_ordering.at(i).preferred_assignment;
      break;
    }
  }
  return chosen_lit;
}

int PreimageSATSolver::propagate(const int lit) {
  std::vector<int> solved_lits;
  std::set<int> &lits_solved_via_propagation = stack.back().implied;
  assert(lit == stack.back().lit_guess);
  assert(lits_solved_via_propagation.size() == 0);

  std::queue<int> q;
  for (int g : lit2gates[lit]) q.push(g);

  while (!q.empty()) {
    const int gate_idx = q.front();
    q.pop();

    const bool ok = partialSolve(gates_[gate_idx], solved_lits);
    if (!ok) return -1;  // conflict

    for (int solved_lit : solved_lits) {
      lits_solved_via_propagation.insert(std::abs(solved_lit));
      for (int g : lit2gates[std::abs(solved_lit)]) {
        if (g != gate_idx) q.push(g);
      }
    }
  }

  return static_cast<int>(lits_solved_via_propagation.size());
}

bool PreimageSATSolver::partialSolve(const LogicGate &g, std::vector<int> &solved_lits) {
  solved_lits.clear();
  switch (g.t()) {
    case LogicGate::Type::and_gate:
      return partialSolveAnd(g, solved_lits);
    case LogicGate::Type::or_gate:
      return partialSolveOr(g, solved_lits);
    case LogicGate::Type::xor_gate:
      return partialSolveXor(g, solved_lits);
    case LogicGate::Type::xor3_gate:
      return partialSolveXor3(g, solved_lits);
    case LogicGate::Type::maj_gate:
      return partialSolveMaj(g, solved_lits);
  }
}

bool PreimageSATSolver::partialSolveAnd(const LogicGate &g,
                                        std::vector<int> &solved_lits) {
  const bool out_known = literals[g.output] != 0;
  const bool inp1_known = literals[std::abs(g.inputs[0])] != 0;
  const bool inp2_known = literals[std::abs(g.inputs[1])] != 0;
  const bool out_val = getLitValue(g.output);
  const bool inp1_val = getLitValue(g.inputs[0]);
  const bool inp2_val = getLitValue(g.inputs[1]);

  if (inp1_known && inp2_known && out_known) {
    return out_val == (inp1_val & inp2_val);
  } else if (inp1_known && inp2_known) {
    setLitValue(g.output, inp1_val & inp2_val);
    solved_lits = {g.output};
    return true;
  } else if ((inp1_known && !inp1_val) || (inp2_known && !inp2_val)) {
    setLitValue(g.output, false);
    solved_lits = {g.output};
    return true;
  } else if (out_known) {
    if (out_val) {
      if (inp1_known && !inp1_val) return false;
      if (inp2_known && !inp2_val) return false;
      setLitValue(g.inputs[0], true);
      setLitValue(g.inputs[1], true);
      if (!inp1_known) solved_lits.push_back(g.inputs[0]);
      if (!inp2_known) solved_lits.push_back(g.inputs[1]);
      return true;
    } else {
      if (inp1_known && inp1_val) {
        setLitValue(g.inputs[1], false);
        solved_lits = {g.inputs[1]};
        return true;
      } else if (inp2_known && inp2_val) {
        setLitValue(g.inputs[0], false);
        solved_lits = {g.inputs[0]};
        return true;
      }
    }
  }
  return true;
}

bool PreimageSATSolver::partialSolveOr(const LogicGate &g,
                                       std::vector<int> &solved_lits) {
  const bool out_known = literals[g.output] != 0;
  const bool inp1_known = literals[std::abs(g.inputs[0])] != 0;
  const bool inp2_known = literals[std::abs(g.inputs[1])] != 0;
  const bool out_val = getLitValue(g.output);
  const bool inp1_val = getLitValue(g.inputs[0]);
  const bool inp2_val = getLitValue(g.inputs[1]);

  if (inp1_known && inp2_known && out_known) {
    return out_val == (inp1_val | inp2_val);
  } else if (inp1_known && inp2_known) {
    setLitValue(g.output, inp1_val | inp2_val);
    solved_lits = {g.output};
    return true;
  } else if ((inp1_known && inp1_val) || (inp2_known && inp2_val)) {
    setLitValue(g.output, true);
    solved_lits = {g.output};
    return true;
  } else if (out_known) {
    if (!out_val) {
      if (inp1_known && inp1_val) return false;
      if (inp2_known && inp2_val) return false;
      setLitValue(g.inputs[0], false);
      setLitValue(g.inputs[1], false);
      if (!inp1_known) solved_lits.push_back(g.inputs[0]);
      if (!inp2_known) solved_lits.push_back(g.inputs[1]);
      return true;
    } else {
      if (inp1_known && !inp1_val) {
        setLitValue(g.inputs[1], true);
        solved_lits = {g.inputs[1]};
        return true;
      } else if (inp2_known && !inp2_val) {
        setLitValue(g.inputs[0], true);
        solved_lits = {g.inputs[0]};
        return true;
      }
    }
  }
  return true;
}

bool PreimageSATSolver::partialSolveXor(const LogicGate &g,
                                        std::vector<int> &solved_lits) {
  const bool out_known = literals[g.output] != 0;
  const bool inp1_known = literals[std::abs(g.inputs[0])] != 0;
  const bool inp2_known = literals[std::abs(g.inputs[1])] != 0;
  const bool out_val = getLitValue(g.output);
  const bool inp1_val = getLitValue(g.inputs[0]);
  const bool inp2_val = getLitValue(g.inputs[1]);
  if (inp1_known && inp2_known && out_known) {
    return out_val == (inp1_val ^ inp2_val);
  } else if (inp1_known && inp2_known) {
    setLitValue(g.output, inp1_val ^ inp2_val);
    solved_lits = {g.output};
    return true;
  } else if (inp1_known && out_known) {
    setLitValue(g.inputs[1], inp1_val ^ out_val);
    solved_lits = {g.inputs[1]};
    return true;
  } else if (inp2_known && out_known) {
    setLitValue(g.inputs[0], inp2_val ^ out_val);
    solved_lits = {g.inputs[0]};
    return true;
  }
  return true;
}

bool PreimageSATSolver::partialSolveXor3(const LogicGate &g,
                                         std::vector<int> &solved_lits) {
  bool known[4];
  int unknown[4];
  uint8_t num_known = 0;
  uint8_t num_unknown = 0;
  for (int inp : g.inputs) {
    if (literals[std::abs(inp)] != 0)
      known[num_known++] = getLitValue(inp);
    else
      unknown[num_unknown++] = inp;
  }
  if (literals[g.output] != 0)
    known[num_known++] = getLitValue(g.output);
  else
    unknown[num_unknown++] = g.output;

  if (num_known == 4) {
    return known[3] == (known[0] ^ known[1] ^ known[2]);
  } else if (num_known == 3) {
    setLitValue(unknown[0], known[0] ^ known[1] ^ known[2]);
    solved_lits = {unknown[0]};
    return true;
  }
  return true;
}

bool PreimageSATSolver::partialSolveMaj(const LogicGate &g,
                                        std::vector<int> &solved_lits) {
  bool known[3];
  int unknown[3];
  uint8_t inp_sum = 0;
  uint8_t num_known_inputs = 0;
  uint8_t num_unknown_inputs = 0;
  for (int inp : g.inputs) {
    if (literals[std::abs(inp)] != 0) {
      known[num_known_inputs] = getLitValue(inp);
      inp_sum += static_cast<uint8_t>(known[num_known_inputs]);
      ++num_known_inputs;
    } else {
      unknown[num_unknown_inputs++] = inp;
    }
  }
  const bool out_known = literals[g.output] != 0;
  const bool out_val = getLitValue(g.output);

  if (num_known_inputs == 3) {
    if (out_known) return out_val == (inp_sum > 1);
    setLitValue(g.output, inp_sum > 1);
    solved_lits = {g.output};
    return true;
  } else if (num_known_inputs == 2) {
    if (known[0] == known[1]) {
      if (out_known) return out_val == known[0];
      setLitValue(g.output, known[0]);
      solved_lits = {g.output};
      return true;
    } else if (out_known) {
      setLitValue(unknown[0], out_val);
      solved_lits = {unknown[0]};
      return true;
    }
  } else if (num_known_inputs == 1 && out_known && known[0] != out_val) {
    setLitValue(unknown[0], out_val);
    setLitValue(unknown[1], out_val);
    solved_lits = {unknown[0], unknown[1]};
    return true;
  }
  return true;
}

}  // end namespace preimage
