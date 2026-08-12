#include "../utils/Env.hpp"

#include <iostream>

int builtin_env(Environment &env) {
    for (const auto &item : env.asList()) {
        std::cout << item << "\n";
    }
    return 0;
}
