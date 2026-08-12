#pragma once

#include <memory>
#include <string>
#include <vector>

enum class RedirectionType { In, Out, Append, Heredoc };

struct Redirection {
    RedirectionType type;
    std::string target;
};

struct Assignment {
    std::string key;
    std::string value;
};

struct Command {
    std::vector<std::string> args;
    std::vector<Redirection> redirections;
    std::vector<Assignment> assignments;
};

struct Pipeline {
    std::vector<Command> commands;
};

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

enum class ExprKind { Pipeline, Sequence, And, Or, Subshell };

struct Expr {
    ExprKind kind;
    Pipeline pipeline;
    ExprPtr left;
    ExprPtr right;
    std::vector<Assignment> assignments;
    bool runInBackground = false;
};
