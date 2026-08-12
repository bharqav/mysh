#include "Shell.hpp"

#include "../builtins/Builtins.hpp"
#include "../executor/Executor.hpp"
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../utils/Jobs.hpp"
#include "../utils/Signal.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

#ifdef USE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

Shell::Shell() : lastStatus_(0) { env_.set("SHELL_STATUS", "0"); }

int Shell::runLine(const std::string &input) {
    if (input.empty()) {
        return lastStatus_;
    }

    try {
        auto tokens = Lexer::tokenize(input, env_);
        auto program = Parser::parse(tokens);
        lastStatus_ = Executor::execute(program, env_);
        env_.set("SHELL_STATUS", std::to_string(lastStatus_));
    } catch (const std::exception &ex) {
        std::cerr << "mysh: " << ex.what() << "\n";
        lastStatus_ = 2;
        env_.set("SHELL_STATUS", "2");
    }

    return lastStatus_;
}

namespace {
std::string joinWords(const std::vector<std::string> &words) {
    std::ostringstream out;
    for (std::size_t i = 0; i < words.size(); ++i) {
        if (i > 0) {
            out << ' ';
        }
        out << words[i];
    }
    return out.str();
}

std::string commandToString(const Command &command) {
    std::ostringstream out;
    for (const auto &assignment : command.assignments) {
        out << assignment.key << '=' << assignment.value << ' ';
    }
    out << joinWords(command.args);
    for (const auto &redirection : command.redirections) {
        const char *symbol = "<";
        if (redirection.type == RedirectionType::Out) {
            symbol = ">";
        } else if (redirection.type == RedirectionType::Append) {
            symbol = ">>";
        } else if (redirection.type == RedirectionType::Heredoc) {
            symbol = "<<";
        }
        if (!command.args.empty() || !command.assignments.empty()) {
            out << ' ';
        }
        out << symbol << ' ' << redirection.target;
    }
    return out.str();
}

std::string commandLabel(const Command &command) {
    if (!command.args.empty()) {
        return command.args.front();
    }
    if (!command.assignments.empty()) {
        return "<assignment>";
    }
    return "<command>";
}

struct ExplainSteps {
    int next = 1;
};

void printStep(ExplainSteps &steps, int indent, const std::string &text) {
    const std::string pad(indent, ' ');
    std::cout << pad << steps.next++ << ". " << text << "\n";
}

void renderExplainNode(const Expr *node, int indent, ExplainSteps &steps);

void renderPipelineExplain(const Pipeline &pipeline, bool background, int indent,
                           ExplainSteps &steps) {
    const std::size_t count = pipeline.commands.size();
    if (count == 0) {
        return;
    }

    if (count > 1) {
        printStep(steps, indent, "Create " + std::to_string(count - 1) + " pipe(s)");
    }

    if (count == 1 && !pipeline.commands[0].args.empty() &&
        Builtins::isBuiltin(pipeline.commands[0].args[0]) && !background) {
        printStep(steps, indent,
                  "Run builtin in parent process: " + commandToString(pipeline.commands[0]));
    } else {
        for (std::size_t i = 0; i < count; ++i) {
            printStep(steps, indent,
                      "Fork child process for: " + commandToString(pipeline.commands[i]));
        }
    }

    for (std::size_t i = 0; i + 1 < count; ++i) {
        printStep(steps, indent,
                  "Connect stdout of " + commandLabel(pipeline.commands[i]) + " to stdin of " +
                      commandLabel(pipeline.commands[i + 1]));
    }

    printStep(steps, indent, "Execute command(s)");

    if (background) {
        printStep(steps, indent, "Return prompt immediately and track as background job");
    } else {
        printStep(steps, indent, "Wait for completion and collect exit status");
    }
}

void renderExplainNode(const Expr *node, int indent, ExplainSteps &steps) {
    if (node == nullptr) {
        return;
    }

    const std::string pad(indent, ' ');
    switch (node->kind) {
    case ExprKind::Pipeline:
        renderPipelineExplain(node->pipeline, node->runInBackground, indent, steps);
        return;
    case ExprKind::Subshell:
        printStep(steps, indent, "Fork subshell process");
        renderExplainNode(node->left.get(), indent + 2, steps);
        return;
    case ExprKind::Sequence:
        printStep(steps, indent, "Run left side of sequence ';'");
        renderExplainNode(node->left.get(), indent + 2, steps);
        printStep(steps, indent, "Run right side of sequence ';'");
        renderExplainNode(node->right.get(), indent + 2, steps);
        return;
    case ExprKind::And:
        printStep(steps, indent, "Evaluate left side of '&&'");
        renderExplainNode(node->left.get(), indent + 2, steps);
        printStep(steps, indent, "If left succeeded, evaluate right side");
        renderExplainNode(node->right.get(), indent + 2, steps);
        return;
    case ExprKind::Or:
        printStep(steps, indent, "Evaluate left side of '||'");
        renderExplainNode(node->left.get(), indent + 2, steps);
        printStep(steps, indent, "If left failed, evaluate right side");
        renderExplainNode(node->right.get(), indent + 2, steps);
        return;
    }
}
} // namespace

int Shell::explainLine(const std::string &input) {
    if (input.empty()) {
        return 0;
    }

    try {
        auto tokens = Lexer::tokenize(input, env_);
        auto program = Parser::parse(tokens);
        std::cout << "execution plan:\n";
        ExplainSteps steps;
        renderExplainNode(program.get(), 2, steps);
        std::cout << "\n";
        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "mysh: " << ex.what() << "\n";
        return 2;
    }
}

void Shell::run() {
    Jobs::initShellProcessGroup(getpgrp());
    Signal::setupInteractiveHandlers();
#ifdef USE_READLINE
    using_history();
    read_history(".mysh_history");
#endif

    bool isInteractive = isatty(STDIN_FILENO);

    while (true) {
        Jobs::reapFinished();
        std::string input;
#ifdef USE_READLINE
        if (isInteractive) {
            Signal::setPrompting(false);
            char* line = readline("mysh> ");
            Signal::setPrompting(true);
            if (line == nullptr) {
                std::cout << "\n";
                break;
            }
            input = line;
            if (!input.empty()) {
                add_history(line);
            }
            free(line);
        } else {
            if (!std::getline(std::cin, input)) {
                break;
            }
        }
#else
        if (isInteractive) {
            std::cout << "mysh> " << std::flush;
        }
        Signal::setPrompting(false);
        if (!std::getline(std::cin, input)) {
            Signal::setPrompting(true);
            if (isInteractive) std::cout << "\n";
            break;
        }
        Signal::setPrompting(true);
#endif

        if (input.empty()) {
            continue;
        }

        runLine(input);
    }
#ifdef USE_READLINE
    write_history(".mysh_history");
#endif
}
