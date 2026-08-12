#pragma once

#include <string>

enum class TokenType {
    WORD,
    ASSIGNMENT,
    PIPE,
    AND_IF,
    OR_IF,
    SEMI,
    AMPERSAND,
    LPAREN,
    RPAREN,
    REDIRECT_IN,
    REDIRECT_OUT,
    APPEND,
    HEREDOC
};

struct Token {
    TokenType type;
    std::string value;
};
