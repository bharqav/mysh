#pragma once

#include "../utils/Env.hpp"

#include <string>
#include <vector>
#include <sys/types.h>

namespace Trace {

struct CommandTiming {
    std::string cmd;
    pid_t pid;
    long long startMs;
    long long durationMs;
};

struct Event {
    std::string description;
    std::vector<CommandTiming> commands;
    pid_t processGroup;
    int exitCode;
    long long elapsedMs;
};

bool enabled(const Environment &env);
std::string mode(const Environment &env);
void emit(const Event &evt, const std::string &mode);

} // namespace Trace
