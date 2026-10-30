// pynum - see pynum.h.
#include "pynum.h"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <xlocale.h>

namespace nvqt {

const char32_t PY_ND_ZEROS[67] = {
    0x660,   0x6F0,   0x7C0,   0x966,   0x9E6,   0xA66,   0xAE6,   0xB66,   0xBE6,   0xC66,   0xCE6,   0xD66,
    0xDE6,   0xE50,   0xED0,   0xF20,   0x1040,  0x1090,  0x17E0,  0x1810,  0x1946,  0x19D0,  0x1A80,  0x1A90,
    0x1B50,  0x1BB0,  0x1C40,  0x1C50,  0xA620,  0xA8D0,  0xA900,  0xA9D0,  0xA9F0,  0xAA50,  0xABF0,  0xFF10,
    0x104A0, 0x10D30, 0x11066, 0x110F0, 0x11136, 0x111D0, 0x112F0, 0x11450, 0x114D0, 0x11650, 0x116C0, 0x11730,
    0x118E0, 0x11950, 0x11C50, 0x11D50, 0x11DA0, 0x11F50, 0x16A60, 0x16AC0, 0x16B50, 0x1D7CE, 0x1D7D8, 0x1D7E2,
    0x1D7EC, 0x1D7F6, 0x1E140, 0x1E2F0, 0x1E4F0, 0x1E950, 0x1FBF0};

namespace {

// Py_UNICODE_ISSPACE for code points >= 127
bool uni_space(char32_t c) {
    return c == 0x85 || c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 ||
           c == 0x202F || c == 0x205F || c == 0x3000;
}

// Py_UNICODE_TODECIMAL for code points >= 127: 0..9, or -1
int uni_decimal(char32_t c) {
    for (char32_t z : PY_ND_ZEROS)
        if (c >= z && c < z + 10) return static_cast<int>(c - z);
    return -1;
}

// Py_ISSPACE (pyctype.h): the C-level white space of PyLong_FromString / float_from_string_inner
bool c_space(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
bool c_digit(char c) { return c >= '0' && c <= '9'; }

bool ieq(const char* s, const char* lower) {     // case-insensitive prefix match of `lower` at s
    for (; *lower; ++s, ++lower)
        if (*s == '\0' || (*s | 0x20) != *lower) return false;
    return true;
}

constexpr unsigned __int128 INT_SAT = static_cast<unsigned __int128>(1) << 120;
constexpr int MAX_STR_DIGITS = 4300;

}  // namespace

std::string py_transform_ascii(const std::u32string& s) {
    std::string out;
    out.reserve(s.size());
    for (char32_t ch : s) {
        if (ch < 127) {
            out += static_cast<char>(ch);
        } else if (uni_space(ch)) {
            out += ' ';
        } else {
            const int d = uni_decimal(ch);
            if (d < 0) {
                out += '?';
                break;
            }
            out += static_cast<char>('0' + d);
        }
    }
    return out;
}

long long PyInt::clamp_ll() const {
    const unsigned __int128 m = mag > static_cast<unsigned __int128>(LLONG_MAX) ? LLONG_MAX : mag;
    return neg ? -static_cast<long long>(m) : static_cast<long long>(m);
}

bool py_int(const std::u32string& s, PyInt* v) {
    // PyLong_FromUnicodeObject(s, 10) -> PyLong_FromString on the transformed text
    const std::string t = py_transform_ascii(s);
    const char* p = t.c_str();
    while (c_space(*p)) ++p;
    PyInt r;
    if (*p == '+' || *p == '-') r.neg = *p++ == '-';
    if (*p == '_') return false;                       // may not start with an underscore
    int digits = 0;
    char prev = '\0';
    while (c_digit(*p) || *p == '_') {
        if (*p == '_') {
            if (prev == '_') return false;             // one underscore at a time
        } else {
            ++digits;
            const unsigned d = static_cast<unsigned>(*p - '0');
            r.mag = r.mag >= INT_SAT ? INT_SAT : std::min<unsigned __int128>(r.mag * 10 + d, INT_SAT);
        }
        prev = *p++;
    }
    if (prev == '_' || digits == 0) return false;      // trailing underscore / no digits
    if (digits > MAX_STR_DIGITS) return false;         // "Exceeds the limit (4300 digits) for integer string conversion"
    while (c_space(*p)) ++p;
    if (*p != '\0') return false;
    if (r.mag == 0) r.neg = false;
    *v = r;
    return true;
}

bool py_float(const std::u32string& s, double* v) {
    // PyFloat_FromString: _Py_string_to_number_with_underscores -> float_from_string_inner -> PyOS_string_to_double
    const std::string t0 = py_transform_ascii(s);
    std::string t;
    t.reserve(t0.size());
    char prev = '\0';
    for (char c : t0) {
        if (c == '_') {
            if (!c_digit(prev)) return false;          // underscores only after digits
        } else {
            if (prev == '_' && !c_digit(c)) return false;   // ... and before digits
            t += c;
        }
        prev = c;
    }
    if (prev == '_') return false;
    size_t a = 0, b = t.size();
    while (a < b && c_space(t[a])) ++a;
    if (a == b) return false;
    while (b > a + 1 && c_space(t[b - 1])) --b;
    const std::string u = t.substr(a, b - a);
    // _Py_dg_strtod's grammar: [sign] (digits [. digits*] | . digits) [(e|E) [sign] digits]
    size_t i = 0;
    if (u[i] == '+' || u[i] == '-') ++i;
    size_t nd = 0;
    while (i < u.size() && c_digit(u[i])) ++i, ++nd;
    if (i < u.size() && u[i] == '.') {
        ++i;
        while (i < u.size() && c_digit(u[i])) ++i, ++nd;
    }
    if (nd > 0) {
        if (i < u.size() && (u[i] == 'e' || u[i] == 'E')) {
            size_t j = i + 1;
            if (j < u.size() && (u[j] == '+' || u[j] == '-')) ++j;
            if (j < u.size() && c_digit(u[j])) {
                while (j < u.size() && c_digit(u[j])) ++j;
                i = j;
            }
        }
        if (i != u.size()) return false;
        // the decimal text is validated: the C library's strtod (correctly rounded, as Python's dtoa) in the C locale
        static locale_t c_loc = newlocale(LC_ALL_MASK, "C", nullptr);
        *v = strtod_l(u.c_str(), nullptr, c_loc);
        return true;
    }
    // _Py_parse_inf_or_nan (the whole text)
    const char* p = u.c_str();
    bool negate = false;
    if (*p == '-' || *p == '+') negate = *p++ == '-';
    double r;
    if (ieq(p, "inf")) {
        p += 3;
        if (ieq(p, "inity")) p += 5;
        r = negate ? -__builtin_inf() : __builtin_inf();
    } else if (ieq(p, "nan")) {
        p += 3;
        r = negate ? -__builtin_nan("") : __builtin_nan("");
    } else {
        return false;
    }
    if (*p != '\0') return false;
    *v = r;
    return true;
}

void py_print_exception(const std::vector<PyFrame>& frames, const std::string& type, const std::string& message) {
    std::fprintf(stderr, "Traceback (most recent call last):\n");
    for (const PyFrame& f : frames) {
        std::fprintf(stderr, "  File \"%s\", line %d, in %s\n", f.file.c_str(), f.line, f.func.c_str());
        if (!f.source.empty()) std::fprintf(stderr, "    %s\n", f.source.c_str());
    }
    if (message.empty())
        std::fprintf(stderr, "%s\n", type.c_str());
    else
        std::fprintf(stderr, "%s: %s\n", type.c_str(), message.c_str());
    std::fflush(stderr);
}

}  // namespace nvqt
