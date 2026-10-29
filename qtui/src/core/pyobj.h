// PyObj - an ordered JSON value with Python's semantics, for the profile dicts of the PySide6 UI
// (nvfilter.build_profile_v5 / nv_aft_v5.apply_aft results) and the files it writes:
//   - objects keep insertion order (a Python dict), "set" of a present key keeps its place;
//   - ints and floats are distinct (json.loads: a number with '.', 'e' or 'E' is a float);
//   - dump() writes what json.dumps / json.dump(indent=1) writes (float repr = Python repr, NaN /
//     Infinity as Python's json module writes them).
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nvqt {

class PyObj {
public:
    enum Type { NONE, BOOL, INT, FLOAT, STR, LIST, DICT };

    PyObj() = default;
    static PyObj none() { return PyObj(); }
    static PyObj boolean(bool v) { PyObj o; o.t_ = BOOL; o.b_ = v; return o; }
    static PyObj integer(int64_t v) { PyObj o; o.t_ = INT; o.i_ = v; return o; }
    static PyObj real(double v) { PyObj o; o.t_ = FLOAT; o.d_ = v; return o; }
    static PyObj str(std::string v) { PyObj o; o.t_ = STR; o.s_ = std::move(v); return o; }
    static PyObj list() { PyObj o; o.t_ = LIST; return o; }
    static PyObj dict() { PyObj o; o.t_ = DICT; return o; }
    static PyObj reals(const std::vector<double>& v);
    static PyObj ints(const std::vector<int>& v);

    Type type() const { return t_; }
    bool is_none() const { return t_ == NONE; }
    bool is_dict() const { return t_ == DICT; }
    bool is_list() const { return t_ == LIST; }
    bool is_num() const { return t_ == INT || t_ == FLOAT || t_ == BOOL; }

    // values (Python's float() / int() / bool() of numbers)
    double as_double() const;
    int64_t as_int() const;
    bool truthy() const;
    const std::string& as_str() const { return s_; }
    std::vector<double> as_doubles() const;

    // list
    size_t size() const { return t_ == LIST ? l_.size() : t_ == DICT ? keys_.size() : 0; }
    const PyObj& operator[](size_t i) const { return l_[i]; }
    PyObj& operator[](size_t i) { return l_[i]; }
    void append(PyObj v) { l_.push_back(std::move(v)); }
    const std::vector<PyObj>& items() const { return l_; }

    // dict
    bool has(const std::string& k) const;
    const PyObj& get(const std::string& k) const;            // none() when missing
    PyObj& at(const std::string& k);                          // inserts none() at the end when missing
    void set(const std::string& k, PyObj v);                  // keeps the place of an existing key
    bool pop(const std::string& k);                           // remove; false when missing
    const std::vector<std::string>& keys() const { return keys_; }

    bool operator==(const PyObj& o) const;
    bool operator!=(const PyObj& o) const { return !(*this == o); }

    // json.dumps(obj) (indent < 0: ", " / ": " separators on one line) or json.dump(obj, fh, indent=n);
    // sort_keys as json.dumps(sort_keys=True).
    std::string dump(int indent = -1, bool sort_keys = false) const;
    // json.loads; throws std::runtime_error on malformed input.
    static PyObj parse(const std::string& text);

private:
    void dump_to(std::string& out, int indent, int level, bool sort_keys) const;

    Type t_ = NONE;
    bool b_ = false;
    int64_t i_ = 0;
    double d_ = 0.0;
    std::string s_;
    std::vector<PyObj> l_;                 // list items, or dict values in key order
    std::vector<std::string> keys_;        // dict keys in insertion order
};

// Python repr(float) (the shortest round-trip digits, exponent form outside 1e-4 <= |x| < 1e16).
std::string py_float_repr(double v);
// Python format(v, ".Nf") / "%.Nf" (correctly rounded, as CPython).
std::string py_fixed(double v, int decimals);

}  // namespace nvqt
