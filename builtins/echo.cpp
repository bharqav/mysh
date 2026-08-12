#include <iostream>
#include <string>
#include <vector>

namespace {
bool isAllN(const std::string& s) {
    if (s.size() < 2 || s[0] != '-') {
        return false;
    }
    for (std::size_t i = 1; i < s.size(); ++i) {
        if (s[i] != 'n') {
            return false;
        }
    }
    return true;
}
} // namespace

int builtin_echo(const std::vector<std::string>& args) {
    bool newline = true;
    std::size_t i = 1;
    while (i < args.size() && isAllN(args[i])) {
        newline = false;
        ++i;
    }
    for (; i < args.size(); ++i) {
        std::cout << args[i];
        if (i + 1 < args.size()) {
            std::cout << " ";
        }
    }
    if (newline) {
        std::cout << "\n";
    }
    return 0;
}
