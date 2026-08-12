#pragma once

#include "Env.hpp"

#include <string>

class Shell {
public:
    Shell();
    void run();
    int runLine(const std::string& input);
    int explainLine(const std::string& input);

private:
    Environment env_;
    int lastStatus_;
};
