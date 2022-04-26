/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include "core/cnf.hpp"

#include <assert.h>

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <utility>

namespace preimage {

struct Simplification {
 public:
  Simplification(const CNF &cnf, const std::unordered_map<int, bool> &assignments) {
    std::vector<std::pair<int, bool>> q;
    std::vector<std::set<int>> tmp_clauses = cnf.clauses;
    std::unordered_map<int, std::set<int>> lit2clauses;
    original_assignments = assignments;

    // Mapping from: literal --> indices of clauses which use that literal
    for (int c = 0; c < cnf.num_clauses; ++c) {
      for (int lit : tmp_clauses[c]) lit2clauses[lit].insert(c);
    }

    // Queue will contain literals for which we KNOW the assignment
    for (const auto &itr : assignments) {
      assert(itr.first != 0);
      q.push_back({itr.first, itr.second});
      q.push_back({-itr.first, !itr.second});
    }

    while (q.size() > 0) {
      // Pop from queue and cache the known assignment
      const int lit = q.back().first;
      const bool val = q.back().second;
      q.pop_back();
      original_assignments[lit] = val;
      original_assignments[-lit] = !val;

      const std::set<int> &clause_indices = lit2clauses[lit];
      if (val) {
        // All referenced clauses are automatically SAT
        for (int clause_idx : clause_indices) tmp_clauses[clause_idx] = {};
      } else {
        for (int clause_idx : clause_indices) {
          if (tmp_clauses[clause_idx].size() == 0) continue;
          // If this is the last literal in the clause, UNSAT
          assert(tmp_clauses[clause_idx].size() != 1);
          // Remove literal from clause, since it is 0 / false
          tmp_clauses[clause_idx].erase(lit);
          if (tmp_clauses[clause_idx].size() == 1) {
            const int last_lit = *(tmp_clauses[clause_idx].begin());
            if (original_assignments.count(last_lit) && !original_assignments[last_lit]) {
              assert(false);  // UNSAT because we need last_lit = 1
            }
            if (original_assignments.count(-last_lit) &&
                original_assignments[-last_lit]) {
              assert(false);  // UNSAT, need -last_lit = 0 --> last_lit = 1
            }
            q.push_back({last_lit, true});
            q.push_back({-last_lit, false});
            tmp_clauses[clause_idx] = {};
          }
        }
      }
    }

    std::unordered_map<int, int> lit_original_to_simplified;
    std::vector<std::set<int>> simplified_clauses;
    int k = 1;

    for (const std::set<int> &orig_clause : tmp_clauses) {
      if (orig_clause.size() == 0) continue;  // Automatically SAT
      std::set<int> simplified_clause;
      for (int orig_lit : orig_clause) {
        if (lit_original_to_simplified.count(orig_lit) == 0) {
          lit_original_to_simplified[std::abs(orig_lit)] = k;
          lit_original_to_simplified[-std::abs(orig_lit)] = -k;
          k++;
        }
        simplified_clause.insert(lit_original_to_simplified[orig_lit]);
      }
      simplified_clauses.push_back(simplified_clause);
    }

    original_cnf = cnf;
    simplified_cnf = CNF(simplified_clauses, k - 1);

    // We COULD use less memory by storing in an array of length "k"
    lit_simplified_to_original = {};
    for (const auto &itr : lit_original_to_simplified) {
      lit_simplified_to_original[itr.second] = itr.first;
    }
  }

  CNF original_cnf, simplified_cnf;
  std::unordered_map<int, int> lit_simplified_to_original;
  std::unordered_map<int, bool> original_assignments;
};

CNF::CNF() : num_vars(0), num_clauses(0), clauses({}) {}

CNF::CNF(const std::vector<LogicGate> &gates) {
  num_vars = 0;
  clauses = {};
  for (const LogicGate &gate : gates) {
    const std::vector<std::vector<int>> gate_clauses = gate.cnf();
    for (const std::vector<int> &gc : gate_clauses) {
      clauses.push_back(std::set<int>(gc.begin(), gc.end()));
      for (int lit : gc) num_vars = std::max(num_vars, std::abs(lit));
    }
  }
  num_clauses = static_cast<int>(clauses.size());
}

CNF::CNF(const std::vector<std::set<int>> &cls, int n_var) {
  num_vars = n_var;
  clauses = cls;
  num_clauses = static_cast<int>(clauses.size());
}

int CNF::numSatClauses(const std::unordered_map<int, bool> &assignments) {
  int num_sat = 0;

  for (const auto &clause : clauses) {
    for (int lit : clause) {
      bool lit_val;
      if (assignments.count(lit))
        lit_val = assignments.at(lit);
      else if (assignments.count(-lit))
        lit_val = !assignments.at(-lit);
      else {
        printf("CNF is missing assignment for %d\n", lit);
        assert(false);
      }

      if (lit_val) {
        num_sat++;
        break;
      }
    }
  }

  return num_sat;
}

double CNF::approximationRatio(const std::unordered_map<int, bool> &assignments) {
  if (num_clauses == 0) return 1.0;
  return numSatClauses(assignments) / static_cast<double>(num_clauses);
}

void CNF::toFile(const std::string &filename) const {
  std::ofstream cnf_file(filename);
  if (!cnf_file.is_open()) {
    printf("Unable to open \"%s\" in write mode.\n", filename.c_str());
    assert(false);
  }

  cnf_file << "p cnf " << num_vars << " " << num_clauses << "\n";
  for (const std::set<int> &clause : clauses) {
    for (int var : clause) cnf_file << var << " ";
    cnf_file << "0\n";
  }

  cnf_file.close();
}

CNF CNF::fromFile(const std::string &filename) {
  std::ifstream cnf_file(filename);
  if (!cnf_file.is_open()) {
    printf("Unable to open \"%s\" in read mode.\n", filename.c_str());
    assert(false);
  }

  std::string line;
  int num_vars = -1;
  int num_clauses = -1;
  std::vector<std::set<int>> clauses;
  std::vector<std::string> parts;

  while (std::getline(cnf_file, line)) {
    boost::algorithm::trim(line);
    if (line.size() == 0 || line[0] == '#') continue;
    if (line.rfind("p cnf ", 0) == 0) {
      line = line.substr(6);
      boost::algorithm::trim(line);
      boost::split(parts, line, isspace);
      num_vars = std::stoi(parts[0]);
      num_clauses = std::stoi(parts[1]);
    } else {
      boost::split(parts, line, isspace);
      std::set<int> clause;
      for (const std::string &s : parts) {
        const int lit = std::stoi(s);
        if (lit != 0) clause.insert(lit);
      }
      clauses.push_back(clause);
    }
  }
  cnf_file.close();

  if (num_vars == -1 || num_clauses == -1) {
    printf("%s\n", "Did not read `p cnf N M`, ensure DIMACS header exists.");
    assert(false);
  }

  return CNF(clauses, num_vars);
}

CNF CNF::simplify(const std::unordered_map<int, bool> &assignments) const {
  return Simplification(*this, assignments).simplified_cnf;
}

}  // end namespace preimage
