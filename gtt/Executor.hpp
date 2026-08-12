#pragma once

#include "AST.hpp"
#include "Env.hpp"

class Executor {
public:
    static int execute(const ExprPtr& root, Environment& env);
};
