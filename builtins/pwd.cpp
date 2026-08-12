#include <iostream>
#include <unistd.h>

int builtin_pwd() {
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        perror("pwd");
        return 1;
    }
    std::cout << cwd << "\n";
    return 0;
}
