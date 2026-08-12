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

int builtin_export(const std::vector<std::string> &args, Environment &env) {
    if (args.size() == 1) {
        for (const auto &item : env.asList()) {
            std::cout << "declare -x " << item << "\n";
        }
        return 0;
    }
    int rc = 0;
    for (std::size_t i = 1; i < args.size(); ++i) {
        std::size_t eq = args[i].find('=');
        if (eq == std::string::npos) {
            if (!isValidIdentifier(args[i])) {
                std::cerr << "export: '" << args[i] << "': not a valid identifier\n";
                rc = 1;
                continue;
            }
            env.set(args[i], "");
            continue;
        }
        std::string key = args[i].substr(0, eq);
        if (!isValidIdentifier(key)) {
            std::cerr << "export: '" << key << "': not a valid identifier\n";
            rc = 1;
            continue;
        }
        env.set(key, args[i].substr(eq + 1));
    }
    return rc;
}
