// pynum - Python's int(str) / float(str) and the traceback form of an uncaught exception, for the places where the
// C++ UI reads text the Python UI hands to int() / float() (the OFX arguments, repro/nv_ui_qt/main.py _ofx_main) and
// for its exits through an exception (the OFX frame reads).  CPython 3.13 semantics, Unicode 15.1.0 (the reference
// Python of the project):
//   - the text is first mapped as _PyUnicode_TransformDecimalAndSpaceToASCII does: code points < 127 as they are, the
//     non-ASCII white space (str.isspace: U+0085, U+00A0, U+1680, U+2000..U+200A, U+2028, U+2029, U+202F, U+205F,
//     U+3000) to ' ', the non-ASCII decimal digits (Unicode Nd: the 67 runs of 10 in PY_ND_ZEROS, 670 code points) to
//     '0'..'9', and the text ends at the first other code point (which then fails as '?');
//   - int: ASCII white space (' ', \t \n \v \f \r) around, an optional sign, decimal digits with single underscores
//     between digits, no prefix (0x10 is invalid in base 10); more than 4300 digits (sys.int_info.default_max_str_digits,
//     underscores not counted, leading zeros counted) is a ValueError as well;
//   - float: underscores only between two digits (removed), white space around, then [sign] digits [. digits] /
//     . digits, an optional exponent e[sign]digits, or inf / infinity / nan in any case with an optional sign; no hex
//     floats, no nan(...); correctly rounded (overflow -> +-inf, underflow -> +-0 or a subnormal, as Python's dtoa).
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace nvqt {

// the first code point of each run of 10 non-ASCII decimal digits (Unicode 15.1.0 Nd, values 0..9)
extern const char32_t PY_ND_ZEROS[67];

// the ASCII text CPython parses for int() / float() of the str `s` (UTF-32 code points)
std::string py_transform_ascii(const std::u32string& s);

// int(s): false where Python raises ValueError.  The value is exact up to |v| < 2^120 and saturates there (beyond
// every size the callers can use).
struct PyInt {
    bool neg = false;
    unsigned __int128 mag = 0;
    bool positive() const { return !neg && mag > 0; }
    long long clamp_ll() const;        // saturated to [-LLONG_MAX, LLONG_MAX]
};
bool py_int(const std::u32string& s, PyInt* v);

// float(s): false where Python raises ValueError
bool py_float(const std::u32string& s, double* v);

// An uncaught Python exception as the interpreter prints it on stderr: "Traceback (most recent call last):", the
// frames (file, line, function, source line), then "Type: message" ("Type" alone for an empty message).
struct PyFrame {
    std::string file;
    int line;
    std::string func;
    std::string source;
};
void py_print_exception(const std::vector<PyFrame>& frames, const std::string& type, const std::string& message);

}  // namespace nvqt
