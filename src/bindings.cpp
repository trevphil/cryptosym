#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <boost/dynamic_bitset.hpp>
#include <sstream>

#include "core/config.hpp"
#include "core/logic_gate.hpp"
#include "core/utils.hpp"

namespace py = pybind11;

namespace pybind11 {

namespace detail {

template <>
struct type_caster<boost::dynamic_bitset<>> {
 public:
  PYBIND11_TYPE_CASTER(boost::dynamic_bitset<>, _("boost::dynamic_bitset<>"));

  bool load(handle src, bool implicit_conversions) {
    // Python --> C++
    PyObject *source = src.ptr();
    PyObject *byte_obj = PyBytes_FromObject(source);
    if (!byte_obj) return false;

    const unsigned int num_bytes = py::len(byte_obj);
    const unsigned int n = num_bytes * 8;
    value = boost::dynamic_bitset<>(n);

    unsigned int bit_idx = 0;
    for (unsigned int byte_idx = 0; byte_idx < num_bytes; ++byte_idx) {
      PyObject *b_obj = PySequence_GetItem(byte_obj, byte_idx);
      if (!b_obj) {
        printf("Failed to get byte %ul of %ul\n", byte_idx, num_bytes);
        return false;
      }
      const unsigned long b = PyLong_AsUnsignedLong(b_obj);
      for (int offset = 0; offset < 8 && bit_idx < n; offset++) {
        value[bit_idx++] = (b >> offset) & 1;
      }
    }

    Py_DECREF(byte_obj);
    return !PyErr_Occurred();
  }

  static handle cast(const boost::dynamic_bitset<> &src, return_value_policy policy,
                     handle parent) {
    // C++ --> Python
    const unsigned int n = static_cast<unsigned int>(src.size());
    if (n == 0) return py::bytes("");

    const unsigned int num_bytes = (n / 8) + (unsigned int)(n % 8 != 0);
    unsigned int bit_idx = 0;
    unsigned char data[num_bytes];

    for (unsigned int byte_idx = 0; byte_idx < num_bytes; ++byte_idx) {
      data[byte_idx] = 0b00000000;
      for (int offset = 0; offset < 8 && bit_idx < n; offset++) {
        data[byte_idx] |= (src[bit_idx++] << offset);
      }
    }

    const std::string s(reinterpret_cast<const char *>(data), num_bytes);
    const py::bytes b = py::bytes(s);
    b.inc_ref();  // TODO - Without this, segfault. With: mem leak?
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
  u.def("random_bits", py::overload_cast<int>(&utils::randomBits), py::arg("n"));
  u.def("random_bits", py::overload_cast<int, unsigned int>(&utils::randomBits),
        py::arg("n"), py::arg("seed_value"));
  u.def("str2bits", &utils::str2bits);
  u.def("hexstr", &utils::hexstr);
  u.def("binstr", &utils::binstr);
  u.def("hex2bits", &utils::hex2bits);

  // Logic gate
  py::class_<LogicGate> gate(m, "LogicGate");
  py::enum_<LogicGate::Type>(gate, "Type")
      .value("and_gate", LogicGate::Type::and_gate)
      .value("xor_gate", LogicGate::Type::xor_gate)
      .value("or_gate", LogicGate::Type::or_gate)
      .value("maj_gate", LogicGate::Type::maj_gate)
      .value("xor3_gate", LogicGate::Type::xor3_gate)
      .export_values();
}

}  // end namespace preimage