#include "../utils/Env.hpp"

#include <iostream>
#include <unistd.h>
#include <vector>

int builtin_cd(const std::vector<std::string> &args, Environment &env) {
    std::string target;
    bool printTarget = false;
    if (args.size() < 2) {
        target = env.get("HOME");
        if (target.empty()) {
            std::cerr << "cd: HOME not set\n";
            return 1;
        }
    } else if (args[1] == "-") {
        target = env.get("OLDPWD");
        if (target.empty()) {
            std::cerr << "cd: OLDPWD not set\n";
            return 1;
        }
        printTarget = true;
    } else {
        target = args[1];
    }

    char oldCwd[4096];
    if (getcwd(oldCwd, sizeof(oldCwd)) == nullptr) {
        oldCwd[0] = '\0';
    }

    if (chdir(target.c_str()) != 0) {
        perror("cd");
        return 1;
    }
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        if (oldCwd[0] != '\0') {
            env.set("OLDPWD", oldCwd);
        }
        env.set("PWD", cwd);
        if (printTarget) {
            std::cout << cwd << "\n";
        }
    }
    return 0;
}
