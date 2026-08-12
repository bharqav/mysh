#pragma once

#include "../parser/AST.hpp"
#include "../utils/Env.hpp"

class Executor {
public:
    static int execute(const ExprPtr& root, Environment& env);
};
