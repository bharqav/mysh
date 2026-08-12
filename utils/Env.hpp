#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class Environment {
public:
    Environment();

    std::string get(const std::string& key) const;
    void set(const std::string& key, const std::string& value);
    void unset(const std::string& key);
    std::vector<std::string> asList() const;
    bool contains(const std::string& key) const;

private:
    std::unordered_map<std::string, std::string> vars_;
};
