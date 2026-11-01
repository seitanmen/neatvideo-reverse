#include <stdexcept>
#include <cstdio>
#include <string>
struct E : std::exception { std::string m; E(std::string s) : m(std::move(s)) {} const char* what() const noexcept override { return m.c_str(); } };
int main(int argc, char**) {
    std::fprintf(stderr, "-- custom type\n");
    try { throw E("x"); } catch (const std::exception& e) { std::fprintf(stderr, "caught %s\n", e.what()); }
    std::fprintf(stderr, "-- std::runtime_error\n");
    try { throw std::runtime_error("y"); } catch (const std::exception& e) { std::fprintf(stderr, "caught %s\n", e.what()); }
    std::fprintf(stderr, "-- std::runtime_error (2nd)\n");
    { std::runtime_error r("z"); }
    return 0;
}
