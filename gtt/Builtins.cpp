#include "Builtins.hpp"

#include "Jobs.hpp"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

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

namespace {
bool isAllN(const std::string& s) {
    if (s.size() < 2 || s[0] != '-') {
        return false;
    }
    for (std::size_t i = 1; i < s.size(); ++i) {
        if (s[i] != 'n') {
            return false;
        }
    }
    return true;
}
} // namespace

int builtin_echo(const std::vector<std::string>& args) {
    bool newline = true;
    std::size_t i = 1;
    while (i < args.size() && isAllN(args[i])) {
        newline = false;
        ++i;
    }
    for (; i < args.size(); ++i) {
        std::cout << args[i];
        if (i + 1 < args.size()) {
            std::cout << " ";
        }
    }
    if (newline) {
        std::cout << "\n";
    }
    return 0;
}

namespace {
bool isValidIdentifier(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    char first = name.front();
    if (!std::isalpha(static_cast<unsigned char>(first)) && first != '_') {
        return false;
    }
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return true;
}
} // namespace

int builtin_export(const std::vector<std::string>& args, Environment& env) {
    if (args.size() == 1) {
        for (const auto& item : env.asList()) {
            std::cout << "declare -x " << item << "\n";
        }
        return 0;
    }
    int rc = 0;
    for (std::size_t i = 1; i < args.size(); ++i) {
        std::size_t eq = args[i].find('=');
        if (eq == std::string::npos) {
            if (!isValidIdentifier(args[i])) {
                std::cerr << "export: '" << args[i] << "': not a valid identifier\n";
                rc = 1;
                continue;
            }
            env.set(args[i], "");
            continue;
        }
        std::string key = args[i].substr(0, eq);
        if (!isValidIdentifier(key)) {
            std::cerr << "export: '" << key << "': not a valid identifier\n";
            rc = 1;
            continue;
        }
        env.set(key, args[i].substr(eq + 1));
    }
    return rc;
}

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

int builtin_pwd() {
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        perror("pwd");
        return 1;
    }
    std::cout << cwd << "\n";
    return 0;
}

int builtin_unset(const std::vector<std::string>& args, Environment& env) {
    int rc = 0;
    for (std::size_t i = 1; i < args.size(); ++i) {
        if (!isValidIdentifier(args[i])) {
            std::cerr << "unset: '" << args[i] << "': not a valid identifier\n";
            rc = 1;
            continue;
        }
        env.unset(args[i]);
    }
    return rc;
}

int builtin_env(Environment& env) {
    for (const auto& item : env.asList()) {
        std::cout << item << "\n";
    }
    return 0;
}

int builtin_help() {
    std::cout << "Built-ins: cd pwd echo export unset env exit help jobs fg bg\n";
    std::cout << "Features: pipes (|), redirections (< > >> <<), env expansion ($VAR, $?)\n";
    std::cout << "Extra: logical operators (&&, ||), grouping with (), wildcard expansion (*, ?, [..])\n";
    std::cout << "Note: stateful built-ins (cd/export/unset/fg/bg/exit) require parent-shell context\n";
    return 0;
}

int builtin_jobs() {
    auto all = Jobs::list();
    for (const auto& job : all) {
        const char* state = "Done";
        if (job.state == JobInfo::State::Running) {
            state = "Running";
        } else if (job.state == JobInfo::State::Stopped) {
            state = "Stopped";
        }
        std::cout << "[" << job.id << "] " << state << " " << job.command << "\n";
    }
    return 0;
}

int builtin_fg(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "fg: usage: fg <job_id>\n";
        return 1;
    }
    try {
        int id = std::stoi(args[1]);
        return Jobs::fg(id);
    } catch (...) {
        std::cerr << "fg: invalid job id\n";
        return 1;
    }
}

int builtin_bg(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "bg: usage: bg <job_id>\n";
        return 1;
    }
    try {
        int id = std::stoi(args[1]);
        return Jobs::bg(id);
    } catch (...) {
        std::cerr << "bg: invalid job id\n";
        return 1;
    }
}

namespace Builtins {
bool isBuiltin(const std::string& cmd) {
    return cmd == "cd" || cmd == "pwd" || cmd == "echo" || cmd == "export" || cmd == "unset" ||
           cmd == "env" || cmd == "exit" || cmd == "help" || cmd == "jobs" || cmd == "fg" ||
           cmd == "bg";
}

int unsupportedInChild(const char* name) {
    std::cerr << name << ": not supported in pipelines/subprocess context\n";
    return 1;
}

int run(const std::vector<std::string>& args, Environment& env, bool inParentContext) {
    if (args.empty()) {
        return 0;
    }
    const std::string& cmd = args[0];
    if (cmd == "cd") {
        return inParentContext ? builtin_cd(args, env) : unsupportedInChild("cd");
    }
    if (cmd == "pwd") {
        return builtin_pwd();
    }
    if (cmd == "echo") {
        return builtin_echo(args);
    }
    if (cmd == "export") {
        return inParentContext ? builtin_export(args, env) : unsupportedInChild("export");
    }
    if (cmd == "unset") {
        return inParentContext ? builtin_unset(args, env) : unsupportedInChild("unset");
    }
    if (cmd == "env") {
        return builtin_env(env);
    }
    if (cmd == "help") {
        return builtin_help();
    }
    if (cmd == "jobs") {
        return builtin_jobs();
    }
    if (cmd == "fg") {
        return inParentContext ? builtin_fg(args) : unsupportedInChild("fg");
    }
    if (cmd == "bg") {
        return inParentContext ? builtin_bg(args) : unsupportedInChild("bg");
    }
    if (cmd == "exit") {
        return inParentContext ? builtin_exit(args) : unsupportedInChild("exit");
    }
    return 1;
}
} // namespace Builtins
