#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <stdlib.h>

#include <sstream>
#include <vector>

#include "core/bit_vec.hpp"
#include "core/config.hpp"
#include "core/logic_gate.hpp"
#include "core/sym_bit_vec.hpp"
#include "core/utils.hpp"

namespace py = pybind11;

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

PYBIND11_MODULE(cryptosym, m) {
  m.doc() = "cryptosym";

  // Configuration
  py::class_<dummy>(m, "config")
      .def_property_static(
          "only_and_gates", [](py::object) { return config::only_and_gates; },
          [](py::object, bool v) { config::only_and_gates = v; })
      .def_property_static(
          "verbose", [](py::object) { return config::verbose; },
          [](py::object, bool v) { config::verbose = v; });

  // Utilities
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

  // Logic gate
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

  // Symbolic bit vector
  py::class_<SymBitVec> bitvec(m, "SymBitVec");
  bitvec.def(py::init());
  bitvec.def(py::init<const BitVec &, bool>(), py::arg("bits"),
             py::arg("unknown") = false);
  bitvec.def(py::init<uint64_t, int, bool>(), py::arg("n"), py::arg("size"),
             py::arg("unknown") = false);
  bitvec.def("__len__", &SymBitVec::size, py::is_operator());
  bitvec.def("__int__", &SymBitVec::intVal, py::is_operator());
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

  // CNF

  // Symbolic representation

  // Symbolic hash function

  // Solver
}

}  // end namespace preimage
