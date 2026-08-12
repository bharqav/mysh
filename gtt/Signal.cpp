#include "Signal.hpp"

#include <csignal>
#include <unistd.h>

namespace {
volatile sig_atomic_t g_prompting = 1;

void handleSigint(int) {
    const char* prompt = "\nmysh> ";
    const char* newline = "\n";
    const char* out = g_prompting ? prompt : newline;
    write(STDOUT_FILENO, out, g_prompting ? 7 : 1);
}
} // namespace

void Signal::setupInteractiveHandlers() {
    std::signal(SIGINT, handleSigint);
    std::signal(SIGQUIT, SIG_IGN);
    std::signal(SIGTSTP, SIG_IGN);
}

void Signal::setPrompting(bool prompting) {
    g_prompting = prompting ? 1 : 0;
}
