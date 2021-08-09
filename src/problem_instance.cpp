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

#include "problem_instance.hpp"

namespace preimage {

ProblemInstance::ProblemInstance(int num_input_bits, int difficulty,
                                 bool verbose, bool bin_format)
    : num_input_bits_(num_input_bits), difficulty_(difficulty),
      verbose_(verbose), bin_format_(bin_format) {}

int ProblemInstance::prepare(const std::string &hash_name,
                             const std::string &solver_name) {
  createHasher(hash_name);
  if (hasher == nullptr) return 1;
  createSolver(solver_name);
  if (solver == nullptr) return 1;
  return 0;
}

int ProblemInstance::execute() {
  if (hasher == nullptr || solver == nullptr) return 1;

  if (difficulty_ == -1) {
    difficulty_ = hasher->defaultDifficulty();
  }

  if (verbose_) {
    spdlog::info("Executing problem instance...");
    spdlog::info("Hash algorithm:\t{}", hasher->hashName());
    spdlog::info("Solver:\t\t{}", solver->solverName());
    spdlog::info("Input message size:\t{} bits", num_input_bits_);
    spdlog::info("Difficulty level:\t{}", difficulty_);
    spdlog::info("-----------------------");
  }

  // Generate symbolic hash function using all-zero inputs
  boost::dynamic_bitset<> zero_input = Utils::zeroBits(num_input_bits_);
  hasher->call(zero_input, difficulty_, true);
  const std::string symbols_file = "/tmp/hash_symbols.txt";
  saveSymbols(symbols_file);

  // Execute hash algorithm on random input
  boost::dynamic_bitset<> real_input = Utils::randomBits(num_input_bits_, 0);
  boost::dynamic_bitset<> real_output = hasher->call(real_input, difficulty_);
  const std::string real_output_hex = Utils::hexstr(real_output);
  const std::string real_output_bin = Utils::binstr(real_output);

  // Free up some memory: clear list of cached logic gates
  LogicGate::reset();

  // Attempt to get an input message which, when hashed, gives `real_output_hex`
  boost::dynamic_bitset<> preimage = getPreimage(symbols_file, real_output_hex);
  boost::dynamic_bitset<> pred_output = hasher->call(preimage, difficulty_);
  const std::string pred_output_hex = Utils::hexstr(pred_output);
  const std::string pred_output_bin = Utils::binstr(pred_output);

  if (verbose_) {
    if (bin_format_) {
      spdlog::info("True input:\t\t{}", Utils::binstr(real_input));
      spdlog::info("Reconstructed input:\t{}", Utils::binstr(preimage));
    } else {
      spdlog::info("True input:\t\t{}", Utils::hexstr(real_input));
      spdlog::info("Reconstructed input:\t{}", Utils::hexstr(preimage));
    }
  }

  // Check if prediction yields the same hash
  if (real_output_hex.compare(pred_output_hex) == 0) {
    if (verbose_) spdlog::info("Success! Hashes match:\t{}", pred_output_hex);
    return 0;
  } else {
    if (verbose_) {
      spdlog::warn("!!! Hashes do not match.");
      spdlog::warn("\tExpected:\t{}", bin_format_ ? real_output_bin : real_output_hex);
      spdlog::warn("\tGot:\t\t{}", bin_format_ ? pred_output_bin : pred_output_hex);
    }
    return 1;
  }
}

void ProblemInstance::createHasher(const std::string &hash_name) {
  if (hash_name.compare("SHA256") == 0) {
    hasher = std::unique_ptr<SymHash>(new SHA256());
  } else if (hash_name.compare("MD5") == 0) {
    hasher = std::unique_ptr<SymHash>(new MD5());
  } else if (hash_name.compare("RIPEMD160") == 0) {
    hasher = std::unique_ptr<SymHash>(new RIPEMD160());
  } else if (hash_name.compare("SameIOHash") == 0) {
    hasher = std::unique_ptr<SymHash>(new SameIOHash());
  } else if (hash_name.compare("NotHash") == 0) {
    hasher = std::unique_ptr<SymHash>(new NotHash());
  } else if (hash_name.compare("LossyPseudoHash") == 0) {
    hasher = std::unique_ptr<SymHash>(new LossyPseudoHash());
  } else if (hash_name.compare("NonLossyPseudoHash") == 0) {
    hasher = std::unique_ptr<SymHash>(new NonLossyPseudoHash());
  } else {
    spdlog::error("Unrecognized hash function: {}", hash_name);
    hasher = nullptr;
  }
}

void ProblemInstance::createSolver(const std::string &solver_name) {
  if (solver_name.compare("cmsat") == 0) {
    solver = std::unique_ptr<Solver>(new CMSatSolver(verbose_));
  } else if (solver_name.compare("simple") == 0) {
    solver = std::unique_ptr<Solver>(new PreimageSATSolver(verbose_));
  } else if (solver_name.compare("bp") == 0) {
    solver = std::unique_ptr<Solver>(new bp::BPSolver(verbose_));
  } else {
    spdlog::error("Unsupported solver: {}", solver_name);
    solver = nullptr;
  }
}

void ProblemInstance::saveSymbols(const std::string &filename) {
  const std::vector<int> inputs = hasher->hashInputIndices();
  const std::vector<int> outputs = hasher->hashOutputIndices();
  const int I = inputs.size();
  const int O = outputs.size();
  const int M = LogicGate::global_gates.size();

  int N = 0;
  for (int out : outputs) N = std::max(N, abs(out));
  for (const LogicGate &g : LogicGate::global_gates) {
    N = std::max(N, abs(g.output));
    for (int inp : g.inputs) N = std::max(N, abs(inp));
  }

  std::ofstream symbols(filename);
  if (!symbols.is_open()) {
    spdlog::error("Unable to open \"{}\" in write mode.", filename);
    assert(false);
  }

  // Write comments
  symbols << "# hash algorithm: " << hasher->hashName() << "\n";
  symbols << "# difficulty: " << difficulty_ << "\n";
  symbols << "# solver: " << solver->solverName() << "\n";
  symbols << "# input message size: " << I << "\n";
  symbols << "# output message size: " << O << "\n";
  symbols << "# number of variables: " << N << "\n";
  symbols << "# number of gates: " << M << "\n";

  // Write header
  symbols << I << " " << O << " " << N << " " << M << "\n";

  // Write input indices
  for (int k = 0; k < I; k++) {
    symbols << inputs.at(k);
    if (k != I - 1) symbols << " ";
  }
  symbols << "\n";

  // Write output indices
  for (int k = 0; k < O; k++) {
    symbols << outputs.at(k);
    if (k != O - 1) symbols << " ";
  }
  symbols << "\n";

  // Write logic gates
  for (const LogicGate &g : LogicGate::global_gates)
    symbols << g.toString() << "\n";

  symbols.close();
  if (verbose_) spdlog::info("Wrote symbols to: \"{}\"", filename);
}

boost::dynamic_bitset<> ProblemInstance::getPreimage(const std::string &symbols_file,
                                                     const std::string &hash_hex) {
  std::ifstream symbols(symbols_file);
  if (!symbols.is_open()) {
    spdlog::error("Unable to open \"{}\" in read mode.", symbols_file);
    assert(false);
  }

  std::string line;
  int I, O, N, M;

  // Skip comments
  while (std::getline(symbols, line) && line[0] == '#') continue;
  // Parse integer header
  std::istringstream headers(line);
  headers >> I >> O >> N >> M;

  std::vector<int> inputs(I);
  std::vector<int> outputs(O);

  // Skip comments
  while (std::getline(symbols, line) && line[0] == '#') continue;
  // Parse inputs
  std::istringstream input_stream(line);
  for (int k = 0; k < I; k++) input_stream >> inputs[k];

  // Skip comments
  while (std::getline(symbols, line) && line[0] == '#') continue;
  // Parse outputs
  std::istringstream output_stream(line);
  for (int k = 0; k < O; k++) output_stream >> outputs[k];

  std::vector<LogicGate> gates(M);
  for (int k = 0; k < M; k++) {
    // Skip comments
    while (std::getline(symbols, line) && line[0] == '#') continue;
    // Create logic gate from string
    gates[k] = LogicGate(line);
  }

  if (verbose_) {
    std::map<LogicGate::Type, int> gate_counts;
    for (const LogicGate &g : gates) gate_counts[g.t()]++;
    const double c = 100.0 / static_cast<double>(gates.size());
    spdlog::info("Logic gate distribution:");
    for (const auto &itr : gate_counts) {
      spdlog::info("\t{}:\t{}\t({:.1f}%)",
                   (char)itr.first, itr.second, itr.second * c);
    }
  }

  // Configure the solver
  solver->setHeader(I, O, N, M);
  solver->setInputIndices(inputs);
  solver->setOutputIndices(outputs);
  solver->setGates(gates);

  // Convert hash (in hexadecimal) to a bitvector
  boost::dynamic_bitset<> output_bits = Utils::hex2bits(hash_hex);
  assert(output_bits.size() == O);

  // Assign hash output bits to a mapping from (index --> value)
  std::unordered_map<int, bool> observed;
  for (int k = 0; k < O; k++) {
    const int output_index = outputs.at(k);
    if (output_index < 0) {
      observed[-output_index] = !output_bits[k];
    } else if (output_index > 0) {
      observed[output_index] = output_bits[k];
    }
  }
  solver->setObserved(observed);

  const std::string cnf_file = "/tmp/hash_cnf.txt";
  const int nc = solver->writeCNF(cnf_file);
  if (verbose_) spdlog::info("Wrote CNF to: \"{}\" (clauses: {})", cnf_file, nc);

  // Solve for all unknown variables!
  // Assume that only non-inverted (positive-indexed) bits are given
  std::unordered_map<int, bool> assignments = solver->solve();

  // Build input bits from the solver assignments
  boost::dynamic_bitset<> input_bits(I);
  for (int k = 0; k < I; k++) {
    const int input_index = inputs.at(k);
    if (input_index < 0 && assignments.count(-input_index) > 0) {
      input_bits[k] = !assignments.at(-input_index);
    } else if (input_index > 0 && assignments.count(input_index) > 0) {
      input_bits[k] = assignments.at(input_index);
    } else {
      input_bits[k] = 0;
    }
  }
  return input_bits;
}

}  // end namespace preimage