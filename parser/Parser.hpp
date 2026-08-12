#pragma once

#include "../lexer/Token.hpp"
#include "AST.hpp"
#include <vector>

class Parser {
public:
    static ExprPtr parse(const std::vector<Token>& tokens);
};
