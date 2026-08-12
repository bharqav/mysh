#pragma once

#include "Env.hpp"
#include "Token.hpp"
#include <string>
#include <vector>

class Lexer {
public:
    static std::vector<Token> tokenize(const std::string& input, const Environment& env);
};
