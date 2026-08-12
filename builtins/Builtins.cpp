#include "Builtins.hpp"

#include <iostream>
#include <string>

int builtin_cd(const std::vector<std::string>& args, Environment& env);
int builtin_echo(const std::vector<std::string>& args);
int builtin_export(const std::vector<std::string>& args, Environment& env);
int builtin_exit(const std::vector<std::string>& args);
int builtin_pwd();
int builtin_unset(const std::vector<std::string>& args, Environment& env);
int builtin_env(Environment& env);
int builtin_help();
int builtin_jobs();
int builtin_fg(const std::vector<std::string>& args);
int builtin_bg(const std::vector<std::string>& args);

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
