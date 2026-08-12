#include <cstdlib>
#include <iostream>
#include <vector>

int builtin_exit(const std::vector<std::string>& args) {
    int code = 0;
    if (args.size() > 1) {
        try {
            code = std::stoi(args[1]);
        } catch (...) {
            std::cerr << "exit: numeric argument required\n";
            code = 2;
        }
    }
    std::exit(code);
}
