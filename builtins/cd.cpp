#include "../utils/Env.hpp"

#include <iostream>
#include <unistd.h>
#include <vector>

int builtin_cd(const std::vector<std::string>& args, Environment& env) {
    const char* target = nullptr;
    bool printTarget = false;
    if (args.size() < 2) {
        std::string home = env.get("HOME");
        if (home.empty()) {
            std::cerr << "cd: HOME not set\n";
            return 1;
        }
        target = home.c_str();
    } else if (args[1] == "-") {
        std::string oldpwd = env.get("OLDPWD");
        if (oldpwd.empty()) {
            std::cerr << "cd: OLDPWD not set\n";
            return 1;
        }
        target = oldpwd.c_str();
        printTarget = true;
    } else {
        target = args[1].c_str();
    }

    char oldCwd[4096];
    if (getcwd(oldCwd, sizeof(oldCwd)) == nullptr) {
        oldCwd[0] = '\0';
    }

    if (chdir(target) != 0) {
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
