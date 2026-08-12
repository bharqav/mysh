#include "../utils/Env.hpp"

#include <cctype>
#include <iostream>
#include <string>
#include <vector>

namespace {
bool isValidIdentifier(const std::string &name) {
    if (name.empty()) {
        return false;
    }
    char first = name.front();
    if (!std::isalpha(static_cast<unsigned char>(first)) && first != '_') {
        return false;
    }
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return true;
}
} // namespace

int builtin_unset(const std::vector<std::string> &args, Environment &env) {
    int rc = 0;
    for (std::size_t i = 1; i < args.size(); ++i) {
        if (!isValidIdentifier(args[i])) {
            std::cerr << "unset: '" << args[i] << "': not a valid identifier\n";
            rc = 1;
            continue;
        }
        env.unset(args[i]);
    }
    return rc;
}
