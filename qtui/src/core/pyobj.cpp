// PyObj - see pyobj.h.
#include "pyobj.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace nvqt {

PyObj PyObj::reals(const std::vector<double>& v) {
    PyObj o = list();
    for (double x : v) o.append(real(x));
    return o;
}

PyObj PyObj::ints(const std::vector<int>& v) {
    PyObj o = list();
    for (int x : v) o.append(integer(x));
    return o;
}

double PyObj::as_double() const {
    switch (t_) {
        case BOOL: return b_ ? 1.0 : 0.0;
        case INT: return static_cast<double>(i_);
        case FLOAT: return d_;
        default: return 0.0;
    }
}

int64_t PyObj::as_int() const {
    switch (t_) {
        case BOOL: return b_ ? 1 : 0;
        case INT: return i_;
        case FLOAT: return static_cast<int64_t>(d_);
        default: return 0;
    }
}

bool PyObj::truthy() const {
    switch (t_) {
        case NONE: return false;
        case BOOL: return b_;
        case INT: return i_ != 0;
        case FLOAT: return d_ != 0.0;
        case STR: return !s_.empty();
        default: return size() != 0;
    }
}

std::vector<double> PyObj::as_doubles() const {
    std::vector<double> out;
    for (const auto& v : l_) out.push_back(v.as_double());
    return out;
}

bool PyObj::has(const std::string& k) const {
    return t_ == DICT && std::find(keys_.begin(), keys_.end(), k) != keys_.end();
}

const PyObj& PyObj::get(const std::string& k) const {
    static const PyObj NONE_OBJ;
    if (t_ != DICT) return NONE_OBJ;
    auto it = std::find(keys_.begin(), keys_.end(), k);
    return it == keys_.end() ? NONE_OBJ : l_[static_cast<size_t>(it - keys_.begin())];
}

PyObj& PyObj::at(const std::string& k) {
    if (t_ != DICT) {
        t_ = DICT;
        keys_.clear();
        l_.clear();
    }
    auto it = std::find(keys_.begin(), keys_.end(), k);
    if (it != keys_.end()) return l_[static_cast<size_t>(it - keys_.begin())];
    keys_.push_back(k);
    l_.emplace_back();
    return l_.back();
}

void PyObj::set(const std::string& k, PyObj v) { at(k) = std::move(v); }

bool PyObj::pop(const std::string& k) {
    if (t_ != DICT) return false;
    auto it = std::find(keys_.begin(), keys_.end(), k);
    if (it == keys_.end()) return false;
    const size_t i = static_cast<size_t>(it - keys_.begin());
    keys_.erase(it);
    l_.erase(l_.begin() + static_cast<long>(i));
    return true;
}

bool PyObj::operator==(const PyObj& o) const {
    // Python ==: numbers compare by value across bool / int / float; dicts ignore key order
    if (is_num() && o.is_num()) {
        if (t_ != FLOAT && o.t_ != FLOAT) return as_int() == o.as_int();
        return as_double() == o.as_double();
    }
    if (t_ != o.t_) return false;
    switch (t_) {
        case NONE: return true;
        case STR: return s_ == o.s_;
        case LIST: return l_ == o.l_;
        case DICT:
            if (keys_.size() != o.keys_.size()) return false;
            for (size_t i = 0; i < keys_.size(); ++i) {
                if (!o.has(keys_[i]) || !(l_[i] == o.get(keys_[i]))) return false;
            }
            return true;
        default: return false;
    }
}

// ------------------------------------------------------------------ float formatting
std::string py_float_str(double v) {
    if (std::isnan(v)) return "nan";
    if (std::isinf(v)) return v > 0 ? "inf" : "-inf";
    return py_float_repr(v);
}

std::string py_float_repr(double v) {
    if (std::isnan(v)) return "NaN";                       // json.dumps spelling
    if (std::isinf(v)) return v > 0 ? "Infinity" : "-Infinity";
    if (v == 0.0) return std::signbit(v) ? "-0.0" : "0.0";
    char buf[64];
    auto r = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::scientific);
    std::string s(buf, r.ptr);
    // s = [-]d[.ddd]e[+-]XX
    bool neg = s[0] == '-';
    if (neg) s.erase(0, 1);
    const size_t e = s.find('e');
    std::string mant = s.substr(0, e);
    const int exp10 = std::stoi(s.substr(e + 1));
    std::string digits;
    for (char c : mant)
        if (c != '.') digits += c;
    const int n = static_cast<int>(digits.size());
    const int decpt = exp10 + 1;
    std::string out;
    if (decpt <= -4 || decpt > 16) {
        out = digits.substr(0, 1);
        if (n > 1) out += "." + digits.substr(1);
        const int x = decpt - 1;
        char eb[16];
        std::snprintf(eb, sizeof(eb), "e%c%02d", x < 0 ? '-' : '+', x < 0 ? -x : x);
        out += eb;
    } else if (decpt <= 0) {
        out = "0." + std::string(static_cast<size_t>(-decpt), '0') + digits;
    } else if (decpt >= n) {
        out = digits + std::string(static_cast<size_t>(decpt - n), '0') + ".0";
    } else {
        out = digits.substr(0, static_cast<size_t>(decpt)) + "." + digits.substr(static_cast<size_t>(decpt));
    }
    return neg ? "-" + out : out;
}

std::string py_fixed(double v, int decimals) {
    if (std::isnan(v)) return "nan";
    if (std::isinf(v)) return v > 0 ? "inf" : "-inf";
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals, v);
    return buf;
}

// ------------------------------------------------------------------ dump
namespace {

void dump_string(std::string& out, const std::string& s) {
    // json.dumps(ensure_ascii=True)
    out += '"';
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == '"') { out += "\\\""; ++i; continue; }
        if (c == '\\') { out += "\\\\"; ++i; continue; }
        if (c == '\n') { out += "\\n"; ++i; continue; }
        if (c == '\r') { out += "\\r"; ++i; continue; }
        if (c == '\t') { out += "\\t"; ++i; continue; }
        if (c == '\b') { out += "\\b"; ++i; continue; }
        if (c == '\f') { out += "\\f"; ++i; continue; }
        if (c < 0x20) {
            char b[8];
            std::snprintf(b, sizeof(b), "\\u%04x", c);
            out += b;
            ++i;
            continue;
        }
        if (c < 0x80) { out += static_cast<char>(c); ++i; continue; }
        // UTF-8 -> code point -> \uXXXX (surrogate pairs above the BMP)
        uint32_t cp = 0;
        int len = (c >= 0xf0) ? 4 : (c >= 0xe0) ? 3 : 2;
        cp = c & (len == 4 ? 0x07 : len == 3 ? 0x0f : 0x1f);
        for (int k = 1; k < len && i + k < s.size(); ++k) cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3f);
        i += static_cast<size_t>(len);
        char b[16];
        if (cp >= 0x10000) {
            cp -= 0x10000;
            std::snprintf(b, sizeof(b), "\\u%04x\\u%04x", 0xd800 + (cp >> 10), 0xdc00 + (cp & 0x3ff));
        } else {
            std::snprintf(b, sizeof(b), "\\u%04x", cp);
        }
        out += b;
    }
    out += '"';
}

}  // namespace

void PyObj::dump_to(std::string& out, int indent, int level, bool sort_keys) const {
    switch (t_) {
        case NONE: out += "null"; return;
        case BOOL: out += b_ ? "true" : "false"; return;
        case INT: out += std::to_string(i_); return;
        case FLOAT: out += py_float_repr(d_); return;
        case STR: dump_string(out, s_); return;
        default: break;
    }
    const bool isd = t_ == DICT;
    const size_t n = isd ? keys_.size() : l_.size();
    if (n == 0) {
        out += isd ? "{}" : "[]";
        return;
    }
    std::vector<size_t> order(n);
    for (size_t i = 0; i < n; ++i) order[i] = i;
    if (isd && sort_keys)
        std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return keys_[a] < keys_[b]; });
    out += isd ? '{' : '[';
    const std::string nl = indent >= 0 ? "\n" + std::string(static_cast<size_t>(indent * (level + 1)), ' ') : "";
    for (size_t j = 0; j < n; ++j) {
        const size_t i = order[j];
        if (j) out += indent >= 0 ? "," : ", ";
        out += nl;
        if (isd) {
            dump_string(out, keys_[i]);
            out += ": ";
        }
        l_[i].dump_to(out, indent, level + 1, sort_keys);
    }
    if (indent >= 0) out += "\n" + std::string(static_cast<size_t>(indent * level), ' ');
    out += isd ? '}' : ']';
}

std::string PyObj::dump(int indent, bool sort_keys) const {
    std::string out;
    dump_to(out, indent, 0, sort_keys);
    return out;
}

// ------------------------------------------------------------------ parse
namespace {

struct Parser {
    const std::string& s;
    size_t i = 0;

    [[noreturn]] void fail(const char* what) {
        throw std::runtime_error(std::string("JSON: ") + what + " at " + std::to_string(i));
    }
    void ws() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) ++i;
    }
    bool lit(const char* t) {
        const size_t n = std::strlen(t);
        if (s.compare(i, n, t) == 0) {
            i += n;
            return true;
        }
        return false;
    }
    static void utf8(std::string& o, uint32_t cp) {
        if (cp < 0x80) o += static_cast<char>(cp);
        else if (cp < 0x800) { o += static_cast<char>(0xc0 | (cp >> 6)); o += static_cast<char>(0x80 | (cp & 0x3f)); }
        else if (cp < 0x10000) {
            o += static_cast<char>(0xe0 | (cp >> 12));
            o += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
            o += static_cast<char>(0x80 | (cp & 0x3f));
        } else {
            o += static_cast<char>(0xf0 | (cp >> 18));
            o += static_cast<char>(0x80 | ((cp >> 12) & 0x3f));
            o += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
            o += static_cast<char>(0x80 | (cp & 0x3f));
        }
    }
    uint32_t hex4() {
        if (i + 4 > s.size()) fail("bad \\u escape");
        uint32_t v = 0;
        for (int k = 0; k < 4; ++k) {
            const char c = s[i++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= static_cast<uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') v |= static_cast<uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= static_cast<uint32_t>(c - 'A' + 10);
            else fail("bad \\u escape");
        }
        return v;
    }
    std::string string() {
        if (s[i] != '"') fail("expected string");
        ++i;
        std::string o;
        while (true) {
            if (i >= s.size()) fail("unterminated string");
            const char c = s[i++];
            if (c == '"') break;
            if (c != '\\') {
                o += c;
                continue;
            }
            if (i >= s.size()) fail("bad escape");
            const char e = s[i++];
            switch (e) {
                case '"': o += '"'; break;
                case '\\': o += '\\'; break;
                case '/': o += '/'; break;
                case 'b': o += '\b'; break;
                case 'f': o += '\f'; break;
                case 'n': o += '\n'; break;
                case 'r': o += '\r'; break;
                case 't': o += '\t'; break;
                case 'u': {
                    uint32_t cp = hex4();
                    if (cp >= 0xd800 && cp < 0xdc00 && s.compare(i, 2, "\\u") == 0) {
                        i += 2;
                        const uint32_t lo = hex4();
                        cp = 0x10000 + ((cp - 0xd800) << 10) + (lo - 0xdc00);
                    }
                    utf8(o, cp);
                    break;
                }
                default: fail("bad escape");
            }
        }
        return o;
    }
    PyObj value() {
        ws();
        if (i >= s.size()) fail("unexpected end");
        const char c = s[i];
        if (c == '{') {
            ++i;
            PyObj o = PyObj::dict();
            ws();
            if (s[i] == '}') { ++i; return o; }
            while (true) {
                ws();
                std::string k = string();
                ws();
                if (s[i] != ':') fail("expected ':'");
                ++i;
                o.set(k, value());     // a repeated key keeps its first place with the last value (Python)
                ws();
                if (s[i] == ',') { ++i; continue; }
                if (s[i] == '}') { ++i; return o; }
                fail("expected ',' or '}'");
            }
        }
        if (c == '[') {
            ++i;
            PyObj o = PyObj::list();
            ws();
            if (s[i] == ']') { ++i; return o; }
            while (true) {
                o.append(value());
                ws();
                if (s[i] == ',') { ++i; continue; }
                if (s[i] == ']') { ++i; return o; }
                fail("expected ',' or ']'");
            }
        }
        if (c == '"') return PyObj::str(string());
        if (lit("null")) return PyObj::none();
        if (lit("true")) return PyObj::boolean(true);
        if (lit("false")) return PyObj::boolean(false);
        if (lit("NaN")) return PyObj::real(std::nan(""));
        if (lit("Infinity")) return PyObj::real(INFINITY);
        if (lit("-Infinity")) return PyObj::real(-INFINITY);
        // number
        const size_t st = i;
        bool isf = false;
        if (s[i] == '-') ++i;
        while (i < s.size() && ((s[i] >= '0' && s[i] <= '9') || s[i] == '.' || s[i] == 'e' || s[i] == 'E' ||
                                s[i] == '+' || s[i] == '-')) {
            if (s[i] == '.' || s[i] == 'e' || s[i] == 'E') isf = true;
            ++i;
        }
        if (i == st) fail("unexpected character");
        const std::string num = s.substr(st, i - st);
        if (isf) {
            double d = 0.0;
            auto r = std::from_chars(num.data(), num.data() + num.size(), d);
            if (r.ec != std::errc() || r.ptr != num.data() + num.size()) fail("bad number");
            return PyObj::real(d);
        }
        int64_t v = 0;
        auto r = std::from_chars(num.data(), num.data() + num.size(), v);
        if (r.ec != std::errc() || r.ptr != num.data() + num.size()) {
            // beyond int64: keep as a float (Python would keep a big int; not a UI value)
            return PyObj::real(std::strtod(num.c_str(), nullptr));
        }
        return PyObj::integer(v);
    }
};

}  // namespace

PyObj PyObj::parse(const std::string& text) {
    Parser p{text};
    PyObj v = p.value();
    p.ws();
    if (p.i != text.size()) p.fail("extra data");
    return v;
}

}  // namespace nvqt
