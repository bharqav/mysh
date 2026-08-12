#include "Env.hpp"

#include <cstdlib>

extern char** environ;

Environment::Environment() {
    for (char** p = environ; p != nullptr && *p != nullptr; ++p) {
        std::string entry(*p);
        std::size_t eq = entry.find('=');
        if (eq != std::string::npos) {
            vars_[entry.substr(0, eq)] = entry.substr(eq + 1);
        }
    }
}

std::string Environment::get(const std::string& key) const {
    auto it = vars_.find(key);
    if (it == vars_.end()) {
        return "";
    }
    return it->second;
}

void Environment::set(const std::string& key, const std::string& value) {
    vars_[key] = value;
    setenv(key.c_str(), value.c_str(), 1);
}

void Environment::unset(const std::string& key) {
    vars_.erase(key);
    unsetenv(key.c_str());
}

std::vector<std::string> Environment::asList() const {
    std::vector<std::string> list;
    list.reserve(vars_.size());
    for (const auto& kv : vars_) {
        list.push_back(kv.first + "=" + kv.second);
    }
    return list;
}

bool Environment::contains(const std::string& key) const {
    return vars_.find(key) != vars_.end();
}
