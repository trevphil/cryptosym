#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <boost/dynamic_bitset.hpp>
#include <sstream>

#include "core/logic_gate.hpp"
#include "core/utils.hpp"

namespace py = pybind11;

int add(int i, int j) { return i + j; }

namespace pybind11 {

namespace detail {

template <>
struct type_caster<boost::dynamic_bitset<>> {
 public:
  PYBIND11_TYPE_CASTER(boost::dynamic_bitset<>, _("boost::dynamic_bitset<>"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject into a inty
   * instance or return false upon failure. The second argument
   * indicates whether implicit conversions should be applied.
   */
  bool load(handle src, bool implicit_conversions) {
    // https://docs.python.org/3.9/c-api/bytes.html?highlight=pybytes#c.PyBytes_FromObject

    /* Extract PyObject from handle */
    PyObject *source = src.ptr();
    /* Try converting into a Python integer value */
    PyObject *tmp_object = PyBytes_FromObject(source);
    if (!tmp_object) {
      printf("%s\n", "Conversion failed!");
      return false;
    }
    const std::string data(PyBytes_AsString(tmp_object));
    printf("Python --> C++: data = %s\n", data.c_str());
    value = boost::dynamic_bitset<>(data);
    Py_DECREF(tmp_object);
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

namespace preimage {

PYBIND11_MODULE(cryptosym, m) {
  m.doc() = "CryptoSym";

  m.def("add", &add, py::arg("i") = 1, py::arg("j") = 2,
        "A function that adds two numbers");
  m.attr("the_answer") = 42;
  py::object world = py::cast("World");
  m.attr("what") = world;

  m.def("seed", &utils::seed, py::arg("seed_value"));
  m.def("zero_bits", &utils::zeroBits, py::arg("n"));
  m.def("random_bits", py::overload_cast<int>(&utils::randomBits), py::arg("n"));
  m.def("random_bits", py::overload_cast<int, unsigned int>(&utils::randomBits),
        py::arg("n"), py::arg("seed_value"));
  m.def("str2bits", &utils::str2bits);
  m.def("hexstr", &utils::hexstr);
  m.def("binstr", &utils::binstr);
  m.def("hex2bits", &utils::hex2bits);

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