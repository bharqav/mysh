#include "core/Shell.hpp"

#include <string>

int main(int argc, char** argv) {
    Shell shell;
    if (argc >= 3 && std::string(argv[1]) == "-c") {
        return shell.runLine(argv[2]);
    }

    if (argc >= 3 && std::string(argv[1]) == "--explain") {
        return shell.explainLine(argv[2]);
    }

    shell.run();
    return 0;
}
