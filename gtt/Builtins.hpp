#pragma once

#include "Env.hpp"
#include <string>
#include <vector>

namespace Builtins {
bool isBuiltin(const std::string& cmd);
int run(const std::vector<std::string>& args, Environment& env, bool inParentContext);
} // namespace Builtins
