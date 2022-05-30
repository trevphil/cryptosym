#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>
#include <stdlib.h>

#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "bp/bp_solver.hpp"
#include "cmsat/cmsat_solver.hpp"
#include "core/bit_vec.hpp"
#include "core/cnf.hpp"
#include "core/config.hpp"
#include "core/logic_gate.hpp"
#include "core/solver.hpp"
#include "core/sym_bit_vec.hpp"
#include "core/sym_hash.hpp"
#include "core/sym_representation.hpp"
#include "core/utils.hpp"
#include "dag_solver/dag_solver.hpp"
#include "hashing/sym_md5.hpp"
#include "hashing/sym_ripemd160.hpp"
#include "hashing/sym_sha256.hpp"

namespace py = pybind11;

/*
 *****************************************************
  C++ `BitVec` <--> Python `bytes` conversion
 *****************************************************
*/

namespace pybind11 {

namespace detail {

template <>
struct type_caster<preimage::BitVec> {
 public:
  PYBIND11_TYPE_CASTER(preimage::BitVec, _("preimage::BitVec"));

  bool load(handle src, bool implicit_conversions) {
    // Python --> C++
    PyObject *source = src.ptr();
    PyObject *byte_obj = PyBytes_FromObject(source);
    if (!byte_obj) return false;

    const unsigned int num_bytes = py::len(byte_obj);
    const unsigned int n = num_bytes * 8;
    value = preimage::BitVec(n);

    unsigned int bit_idx = 0;
    for (unsigned int byte_idx = 0; byte_idx < num_bytes; ++byte_idx) {
      PyObject *b_obj = PySequence_GetItem(byte_obj, byte_idx);
      if (!b_obj) {
        printf("Failed to get byte %ul of %ul\n", byte_idx, num_bytes);
        return false;
      }
      const unsigned long b = PyLong_AsUnsignedLong(b_obj);
      for (int offset = 0; offset < 8 && bit_idx < n; offset++) {
        value[bit_idx++] = static_cast<bool>((b >> offset) & 1);
      }
    }

    Py_DECREF(byte_obj);
    return !PyErr_Occurred();
  }

  static handle cast(const preimage::BitVec &src, return_value_policy policy,
                     handle parent) {
    // C++ --> Python
    const unsigned int n = src.size();
    if (n == 0) return py::bytes("");

    const unsigned int num_bytes = (n / 8) + (unsigned int)(n % 8 != 0);
    unsigned int bit_idx = 0;
    unsigned char *data =
        reinterpret_cast<unsigned char *>(calloc(num_bytes, sizeof(unsigned char)));

    for (unsigned int byte_idx = 0; byte_idx < num_bytes; ++byte_idx) {
      data[byte_idx] = 0b00000000;
      for (int offset = 0; offset < 8 && bit_idx < n; offset++) {
        data[byte_idx] |= (src[bit_idx++] << offset);
      }
    }

    const std::string s(reinterpret_cast<const char *>(data), num_bytes);
    const py::bytes b = py::bytes(s);
    b.inc_ref();  // TODO - Without this, segfault. With: mem leak?
    free(data);
    return b.ptr();
  }
};

}  // end namespace detail

}  // end namespace pybind11

class dummy {};  // dummy class

namespace preimage {

/*
 *****************************************************
  SymHash trampoline class
 *****************************************************
*/

class SymHashTrampoline : public SymHash {
 public:
  // Inherit the constructor
  using SymHash::SymHash;

  // Trampoline (need one for each virtual function)
  int defaultDifficulty() const override {
    PYBIND11_OVERRIDE_PURE_NAME(int,                  /* Return type */
                                SymHash,              /* Parent class */
                                "default_difficulty", /* Name of function in Python */
                                defaultDifficulty,    /* Name of function in C++ */
    );
  }

  std::string hashName() const override {
    // Note the trailing comma! Use trailing comma if function has 0 arguments
    PYBIND11_OVERRIDE_PURE_NAME(std::string, SymHash, "hash_name", hashName, );
  }

  SymBitVec forward(const SymBitVec &hash_input) override {
    PYBIND11_OVERRIDE_PURE(SymBitVec, SymHash, forward, hash_input);
  }
};

/*
 *****************************************************
  Solver trampoline class
 *****************************************************
*/

class SolverTrampoline : public Solver {
 public:
  // Inherit the constructor
  using Solver::Solver;
  // Use this to fix PyBind11 bug in OVERRIDE macro (related to < >)
  using VarDict = std::unordered_map<int, bool>;

  std::unordered_map<int, bool> solve(const SymRepresentation &problem,
                                      const std::string &hash_hex) override {
    PYBIND11_OVERRIDE(VarDict, Solver, solve, problem, hash_hex);
  }

  std::unordered_map<int, bool> solve(const SymRepresentation &problem,
                                      const BitVec &hash_output) override {
    PYBIND11_OVERRIDE(VarDict, Solver, solve, problem, hash_output);
  }

  std::unordered_map<int, bool> solve(
      const SymRepresentation &problem,
      const std::unordered_map<int, bool> &bit_assignments) override {
    PYBIND11_OVERRIDE_PURE(VarDict, Solver, solve, problem, bit_assignments);
  }

  std::string solverName() const override {
    PYBIND11_OVERRIDE_PURE_NAME(std::string, Solver, "solver_name", solverName, );
  }
};  // end class SolverTrampoline

/*
 *****************************************************
  PyBind11 Module
 *****************************************************
*/

PYBIND11_MODULE(_cpp, m) {
  /*
   *****************************************************
    Configuration
   *****************************************************
   */

  py::class_<dummy>(m, "config")
      .def_property_static(
          "only_and_gates", [](py::object) { return config::only_and_gates; },
          [](py::object, bool v) { config::only_and_gates = v; })
      .def_property_static(
          "verbose", [](py::object) { return config::verbose; },
          [](py::object, bool v) { config::verbose = v; });

  /*
   *****************************************************
    Utilities
   *****************************************************
   */

  py::module u = m.def_submodule("utils");
  u.def("seed", &utils::seed, py::arg("seed_value"));
  u.def("zero_bits", &utils::zeroBits, py::arg("n"));
  u.def("random_bits", py::overload_cast<unsigned int>(&utils::randomBits), py::arg("n"));
  u.def("random_bits", py::overload_cast<unsigned int, unsigned int>(&utils::randomBits),
        py::arg("n"), py::arg("seed_value"));
  u.def("str2bits", &utils::str2bits, py::arg("s"));
  u.def("hexstr", &utils::hexstr, py::arg("raw_bytes"));
  u.def("binstr", &utils::binstr, py::arg("raw_bytes"));
  u.def("hex2bits", &utils::hex2bits, py::arg("hex_str"));

  /*
   *****************************************************
    Logic gate
   *****************************************************
   */

  py::class_<LogicGate> gate(m, "LogicGate");
  py::enum_<LogicGate::Type>(gate, "Type")
      .value("and_gate", LogicGate::Type::and_gate)
      .value("xor_gate", LogicGate::Type::xor_gate)
      .value("or_gate", LogicGate::Type::or_gate)
      .value("maj3_gate", LogicGate::Type::maj3_gate)
      .value("xor3_gate", LogicGate::Type::xor3_gate)
      .export_values();
  gate.def(py::init(&LogicGate::fromString), py::arg("string"));
  gate.def(py::init<LogicGate::Type, int, const std::vector<int> &>(), py::arg("t"),
           py::arg("output"), py::arg("inputs") = py::list());
  gate.def("__repr__", &LogicGate::toString);
  gate.def_property_readonly("t", &LogicGate::t);
  gate.def_property_readonly("output", [](const LogicGate &g) { return g.output; });
  gate.def_property_readonly("inputs", [](const LogicGate &g) { return g.inputs; });
  gate.def_property_readonly("gate_type", [](const LogicGate &g) {
    return LogicGate::humanReadableType(g.t());
  });
  gate.def("cnf", &LogicGate::cnf);

  /*
   *****************************************************
    SymBitVec
   *****************************************************
   */

  py::class_<SymBitVec> bitvec(m, "SymBitVec");
  bitvec.def(py::init());
  bitvec.def(py::init<const BitVec &, bool>(), py::arg("byte_data"),
             py::arg("unknown") = false);
  bitvec.def(py::init<uint64_t, int, bool>(), py::arg("n"), py::arg("size"),
             py::arg("unknown") = false);
  bitvec.def("__len__", &SymBitVec::size, py::is_operator());
  bitvec.def("__int__", &SymBitVec::intVal, py::is_operator());
  bitvec.def_property_readonly("num_bytes", [](const SymBitVec &bv) -> unsigned int {
    return (bv.size() / 8) + static_cast<unsigned int>((bv.size() % 8) != 0);
  });
  bitvec.def("bits", &SymBitVec::bits);
  bitvec.def("bin", &SymBitVec::bin);
  bitvec.def("hex", &SymBitVec::hex);
  bitvec.def("__getitem__", [](const SymBitVec &bv, int i) {
    if (i < 0) {
      i = static_cast<int>(bv.size()) + i;
    }
    return bv.at(i).val;
  });
  bitvec.def("__getitem__", [](const SymBitVec &b, const py::slice &slice) -> SymBitVec {
    size_t start = 0, stop = 0, step = 0, slicelength = 0;
    if (!slice.compute(b.size(), &start, &stop, &step, &slicelength)) {
      throw py::error_already_set();
    } else if (step != 1) {
      char err_msg[128];
      const int step_i = static_cast<int>(step);
      snprintf(err_msg, 128, "Slice step size must be 1 (got %d)", step_i);
      throw py::index_error(err_msg);
    }
    const unsigned int start_ui = static_cast<unsigned int>(start);
    const unsigned int stop_ui = static_cast<unsigned int>(stop);
    return b.extract(start_ui, stop_ui);
  });
  bitvec.def("concat", &SymBitVec::concat, py::arg("other"));
  bitvec.def("resize", &SymBitVec::resize, py::arg("n"));
  bitvec.def("rotr", &SymBitVec::rotr, py::arg("n"));
  bitvec.def("rotl", &SymBitVec::rotl, py::arg("n"));
  bitvec.def("__reversed__", &SymBitVec::reversed);
  bitvec.def("reversed_bytes", &SymBitVec::reversedBytes);
  bitvec.def(~py::self);
  bitvec.def(py::self & py::self);
  bitvec.def(py::self ^ py::self);
  bitvec.def(py::self | py::self);
  bitvec.def(py::self + py::self);
  bitvec.def(py::self == py::self);
  bitvec.def(py::self != py::self);
  bitvec.def("__lshift__", &SymBitVec::operator<<);
  bitvec.def("__rshift__", &SymBitVec::operator>>);
  bitvec.def_static("maj3", &SymBitVec::maj3, py::arg("a"), py::arg("b"), py::arg("c"));
  bitvec.def_static("xor3", &SymBitVec::xor3, py::arg("a"), py::arg("b"), py::arg("c"));

  /*
   *****************************************************
    CNF
   *****************************************************
   */

  py::class_<CNF> cnf(m, "CNF");
  cnf.def(py::init());
  cnf.def(py::init<const std::vector<LogicGate> &>(), py::arg("gates"));
  cnf.def(py::init<const std::vector<std::set<int>> &, int>(), py::arg("clauses"),
          py::arg("num_vars"));
  cnf.def(py::init(&CNF::fromFile), py::arg("file_path"));
  cnf.def(
      py::init([](const std::filesystem::path &p) { return CNF::fromFile(p.string()); }),
      py::arg("file_path"));
  cnf.def("num_sat_clauses", &CNF::numSatClauses, py::arg("assignments"));
  cnf.def("approximation_ratio", &CNF::approximationRatio, py::arg("assignments"));
  cnf.def("simplify", &CNF::simplify, py::arg("assignments"));
  cnf.def("to_file", &CNF::toFile, py::arg("file_path"));
  cnf.def(
      "to_file",
      [](const CNF &c, const std::filesystem::path &p) { c.toFile(p.string()); },
      py::arg("file_path"));
  cnf.def_property_readonly("num_vars", [](const CNF &c) { return c.num_vars; });
  cnf.def_property_readonly("num_clauses", [](const CNF &c) { return c.num_clauses; });
  cnf.def_property_readonly("clauses", [](const CNF &c) { return c.clauses; });

  /*
   *****************************************************
    SymRepresentation
   *****************************************************
   */

  py::class_<SymRepresentation> rep(m, "SymRepresentation");
  rep.def(py::init<const std::vector<LogicGate> &, const std::vector<int> &,
                   const std::vector<int> &>(),
          py::arg("gates"), py::arg("input_indices"), py::arg("output_indices"));
  rep.def(py::init(&SymRepresentation::fromDAG), py::arg("dag_file"));
  rep.def(py::init([](const std::filesystem::path &p) {
            return SymRepresentation::fromDAG(p.string());
          }),
          py::arg("dag_file"));
  rep.def_property_readonly("num_vars", &SymRepresentation::numVars);
  rep.def_property_readonly("gates", &SymRepresentation::gates);
  rep.def_property_readonly("input_indices", &SymRepresentation::inputIndices);
  rep.def_property_readonly("output_indices", &SymRepresentation::outputIndices);
  rep.def("to_dag", &SymRepresentation::toDAG, py::arg("file_path"));
  rep.def(
      "to_dag",
      [](const SymRepresentation &s, const std::filesystem::path &p) {
        s.toDAG(p.string());
      },
      py::arg("file_path"));
  rep.def("to_cnf", &SymRepresentation::toCNF);

  /*
   *****************************************************
    SymHash
   *****************************************************
   */

  py::class_<SymHash, SymHashTrampoline> sym_hash(m, "SymHash");
  sym_hash.def(py::init<int, int>(), py::arg("num_input_bits"),
               py::arg("difficulty") = -1);
  sym_hash.def_property_readonly("num_input_bits", &SymHash::numInputBits);
  sym_hash.def_property_readonly("difficulty", &SymHash::difficulty);
  sym_hash.def("forward", &SymHash::forward, py::arg("hash_input"));
  sym_hash.def("__call__", &SymHash::call, py::arg("hash_input"));
  sym_hash.def("__call__", &SymHash::callRandom);
  sym_hash.def("symbolic_representation", &SymHash::getSymbolicRepresentation);
  sym_hash.def("default_difficulty", &SymHash::defaultDifficulty);
  sym_hash.def("hash_name", &SymHash::hashName);

  /*
   *****************************************************
    SymMD5
   *****************************************************
   */

  py::class_<SymMD5, SymHash> sym_md5(m, "SymMD5");
  sym_md5.def(py::init<int, int>(), py::arg("num_input_bits"),
              py::arg("difficulty") = -1);
  sym_md5.def_property_readonly("num_input_bits", &SymMD5::numInputBits);
  sym_md5.def_property_readonly("difficulty", &SymMD5::difficulty);
  sym_md5.def("forward", &SymMD5::forward, py::arg("hash_input"));
  sym_md5.def("__call__", &SymMD5::call, py::arg("hash_input"));
  sym_md5.def("__call__", &SymMD5::callRandom);
  sym_md5.def("symbolic_representation", &SymMD5::getSymbolicRepresentation);
  sym_md5.def("default_difficulty", &SymMD5::defaultDifficulty);
  sym_md5.def("hash_name", &SymMD5::hashName);

  /*
   *****************************************************
    SymRIPEMD160
   *****************************************************
   */

  py::class_<SymRIPEMD160, SymHash> sym_ripemd160(m, "SymRIPEMD160");
  sym_ripemd160.def(py::init<int, int>(), py::arg("num_input_bits"),
                    py::arg("difficulty") = -1);
  sym_ripemd160.def_property_readonly("num_input_bits", &SymRIPEMD160::numInputBits);
  sym_ripemd160.def_property_readonly("difficulty", &SymRIPEMD160::difficulty);
  sym_ripemd160.def("forward", &SymRIPEMD160::forward, py::arg("hash_input"));
  sym_ripemd160.def("__call__", &SymRIPEMD160::call, py::arg("hash_input"));
  sym_ripemd160.def("__call__", &SymRIPEMD160::callRandom);
  sym_ripemd160.def("symbolic_representation", &SymRIPEMD160::getSymbolicRepresentation);
  sym_ripemd160.def("default_difficulty", &SymRIPEMD160::defaultDifficulty);
  sym_ripemd160.def("hash_name", &SymRIPEMD160::hashName);

  /*
   *****************************************************
    SymSHA256
   *****************************************************
   */

  py::class_<SymSHA256, SymHash> sym_sha256(m, "SymSHA256");
  sym_sha256.def(py::init<int, int>(), py::arg("num_input_bits"),
                 py::arg("difficulty") = -1);
  sym_sha256.def_property_readonly("num_input_bits", &SymSHA256::numInputBits);
  sym_sha256.def_property_readonly("difficulty", &SymSHA256::difficulty);
  sym_sha256.def("forward", &SymSHA256::forward, py::arg("hash_input"));
  sym_sha256.def("__call__", &SymSHA256::call, py::arg("hash_input"));
  sym_sha256.def("__call__", &SymSHA256::callRandom);
  sym_sha256.def("symbolic_representation", &SymSHA256::getSymbolicRepresentation);
  sym_sha256.def("default_difficulty", &SymSHA256::defaultDifficulty);
  sym_sha256.def("hash_name", &SymSHA256::hashName);

  /*
   *****************************************************
    Solver
   *****************************************************
   */

  py::class_<Solver, SolverTrampoline> solver(m, "Solver");
  solver.def(py::init());
  solver.def(
      "solve",
      py::overload_cast<const SymRepresentation &, const std::string &>(&Solver::solve),
      py::arg("problem"), py::arg("hash_hex"));
  solver.def("solve",
             py::overload_cast<const SymRepresentation &, const BitVec &>(&Solver::solve),
             py::arg("problem"), py::arg("hash_output"));
  solver.def(
      "solve",
      py::overload_cast<const SymRepresentation &, const std::unordered_map<int, bool> &>(
          &Solver::solve),
      py::arg("problem"), py::arg("bit_assignments"));
  solver.def("solver_name", &Solver::solverName);

  /*
   *****************************************************
    DAGSolver
   *****************************************************
   */

  py::class_<DAGSolver, Solver> dag(m, "DAGSolver");
  dag.def(py::init());
  dag.def("solve",
          py::overload_cast<const SymRepresentation &, const std::string &>(
              &DAGSolver::solve),
          py::arg("problem"), py::arg("hash_hex"));
  dag.def("solve",
          py::overload_cast<const SymRepresentation &, const BitVec &>(&DAGSolver::solve),
          py::arg("problem"), py::arg("hash_output"));
  dag.def(
      "solve",
      py::overload_cast<const SymRepresentation &, const std::unordered_map<int, bool> &>(
          &DAGSolver::solve),
      py::arg("problem"), py::arg("bit_assignments"));
  dag.def("solver_name", &DAGSolver::solverName);
}

}  // end namespace preimage
