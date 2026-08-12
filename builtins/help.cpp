#include <iostream>

int builtin_help() {
    std::cout << "Built-ins: cd pwd echo export unset env exit help jobs fg bg\n";
    std::cout << "Features: pipes (|), redirections (< > >> <<), env expansion ($VAR, $?)\n";
    std::cout << "Extra: logical operators (&&, ||), grouping with (), wildcard expansion (*, ?, [..])\n";
    std::cout << "Note: stateful built-ins (cd/export/unset/fg/bg/exit) require parent-shell context\n";
    return 0;
}
