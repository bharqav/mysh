#include "Executor.hpp"
#include "Trace.hpp"

#include "../builtins/Builtins.hpp"
#include "../utils/Jobs.hpp"
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <glob.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <signal.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace {
struct ScopedAssignments {
    explicit ScopedAssignments(Environment &env, const std::vector<Assignment> &assignments)
        : env_(env) {
        for (const auto &assignment : assignments) {
            if (seen_.find(assignment.key) == seen_.end()) {
                if (env_.contains(assignment.key)) {
                    saved_.push_back({assignment.key, env_.get(assignment.key), true});
                } else {
                    saved_.push_back({assignment.key, std::string{}, false});
                }
                seen_.insert(assignment.key);
            }
            env_.set(assignment.key, assignment.value);
        }
    }

    ~ScopedAssignments() {
        for (auto it = saved_.rbegin(); it != saved_.rend(); ++it) {
            if (it->hadValue) {
                env_.set(it->key, it->value);
            } else {
                env_.unset(it->key);
            }
        }
    }

  private:
    struct SavedValue {
        std::string key;
        std::string value;
        bool hadValue;
    };

    Environment &env_;
    std::vector<SavedValue> saved_;
    std::unordered_set<std::string> seen_;
};

bool isAssignmentOnlyCommand(const Command &cmd) {
    return cmd.args.empty() && !cmd.assignments.empty();
}

void applyAssignments(Environment &env, const std::vector<Assignment> &assignments) {
    for (const auto &assignment : assignments) {
        env.set(assignment.key, assignment.value);
    }
}
} // namespace

namespace {
std::vector<std::string> expandWildcards(const std::vector<std::string> &args);

int applyRedirections(const Command &cmd) {
    for (const auto &redir : cmd.redirections) {
        int fd = -1;
        if (redir.type == RedirectionType::In) {
            fd = open(redir.target.c_str(), O_RDONLY);
            if (fd < 0) {
                perror("open");
                return 1;
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("dup2");
                close(fd);
                return 1;
            }
        } else if (redir.type == RedirectionType::Out) {
            fd = open(redir.target.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open");
                return 1;
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2");
                close(fd);
                return 1;
            }
        } else if (redir.type == RedirectionType::Append) {
            fd = open(redir.target.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror("open");
                return 1;
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2");
                close(fd);
                return 1;
            }
        } else if (redir.type == RedirectionType::Heredoc) {
            int pfd[2];
            if (pipe(pfd) != 0) {
                perror("pipe");
                return 1;
            }
            std::string line;
            while (true) {
                std::cout << "> " << std::flush;
                if (!std::getline(std::cin, line) || line == redir.target) {
                    break;
                }
                line.push_back('\n');
                ssize_t bytesWritten = write(pfd[1], line.c_str(), line.size());
                if (bytesWritten < 0 || static_cast<std::size_t>(bytesWritten) != line.size()) {
                    perror("write");
                    close(pfd[0]);
                    close(pfd[1]);
                    return 1;
                }
            }
            close(pfd[1]);
            if (dup2(pfd[0], STDIN_FILENO) < 0) {
                perror("dup2");
                close(pfd[0]);
                return 1;
            }
            close(pfd[0]);
            continue;
        }
        close(fd);
    }
    return 0;
}

int runBuiltinInParentWithRedirections(const Command &cmd, Environment &env) {
    std::vector<std::string> args = expandWildcards(cmd.args);

    int stdinBackup = dup(STDIN_FILENO);
    int stdoutBackup = dup(STDOUT_FILENO);
    if (stdinBackup < 0 || stdoutBackup < 0) {
        perror("dup");
        if (stdinBackup >= 0) {
            close(stdinBackup);
        }
        if (stdoutBackup >= 0) {
            close(stdoutBackup);
        }
        return 1;
    }

    if (applyRedirections(cmd) != 0) {
        dup2(stdinBackup, STDIN_FILENO);
        dup2(stdoutBackup, STDOUT_FILENO);
        close(stdinBackup);
        close(stdoutBackup);
        return 1;
    }

    int code = Builtins::run(args, env, true);

    if (dup2(stdinBackup, STDIN_FILENO) < 0 || dup2(stdoutBackup, STDOUT_FILENO) < 0) {
        perror("dup2");
    }
    close(stdinBackup);
    close(stdoutBackup);
    return code;
}
} // namespace

namespace {
bool handoffTerminalTo(pid_t pgid) {
    if (!isatty(STDIN_FILENO) || pgid <= 0) {
        return false;
    }
    if (tcsetpgrp(STDIN_FILENO, pgid) < 0) {
        perror("tcsetpgrp");
        return false;
    }
    return true;
}

std::vector<std::string> expandWildcards(const std::vector<std::string> &args) {
    std::vector<std::string> expanded;
    for (const auto &arg : args) {
        if (arg.find('*') == std::string::npos && arg.find('?') == std::string::npos &&
            arg.find('[') == std::string::npos) {
            expanded.push_back(arg);
            continue;
        }
        glob_t matches{};
        int rc = glob(arg.c_str(), 0, nullptr, &matches);
        if (rc == 0) {
            for (std::size_t i = 0; i < matches.gl_pathc; ++i) {
                expanded.push_back(matches.gl_pathv[i]);
            }
        } else {
            expanded.push_back(arg);
        }
        globfree(&matches);
    }
    return expanded;
}

std::string pipelineToString(const Pipeline &pipeline) {
    std::string text;
    for (std::size_t i = 0; i < pipeline.commands.size(); ++i) {
        for (const auto &assignment : pipeline.commands[i].assignments) {
            if (!text.empty()) {
                text += " ";
            }
            text += assignment.key + "=" + assignment.value;
        }
        for (std::size_t j = 0; j < pipeline.commands[i].args.size(); ++j) {
            if (!text.empty()) {
                text += " ";
            }
            text += pipeline.commands[i].args[j];
        }
        if (i + 1 < pipeline.commands.size()) {
            text += " |";
        }
    }
    return text;
}

int executePipeline(const Pipeline &pipeline, Environment &env, bool background) {
    if (pipeline.commands.empty()) {
        return 0;
    }

    const Command &firstCommand = pipeline.commands.front();
    if (pipeline.commands.size() == 1 && isAssignmentOnlyCommand(firstCommand)) {
        applyAssignments(env, firstCommand.assignments);
        return 0;
    }

    ScopedAssignments assignments(env, firstCommand.assignments);

    if (!background && pipeline.commands.size() == 1 && !pipeline.commands[0].args.empty() &&
        Builtins::isBuiltin(pipeline.commands[0].args[0])) {
        return runBuiltinInParentWithRedirections(pipeline.commands[0], env);
    }

    bool doTrace = Trace::enabled(env);
    std::string mode = Trace::mode(env);
    auto traceStartTime = std::chrono::steady_clock::now();
    Trace::Event evt;
    evt.description = pipelineToString(pipeline);

    int prevRead = -1;
    int lastStatus = 0;
    pid_t processGroup = -1;
    pid_t lastCommandPid = -1;

    for (std::size_t i = 0; i < pipeline.commands.size(); ++i) {
        auto commandStart = std::chrono::steady_clock::now();
        int pipefd[2] = {-1, -1};
        if (i + 1 < pipeline.commands.size() && pipe(pipefd) != 0) {
            perror("pipe");
            return 1;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            if (processGroup == -1) {
                setpgid(0, 0);
            } else {
                setpgid(0, processGroup);
            }
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            if (prevRead != -1) {
                dup2(prevRead, STDIN_FILENO);
            }
            if (pipefd[1] != -1) {
                dup2(pipefd[1], STDOUT_FILENO);
            }
            if (prevRead != -1) {
                close(prevRead);
            }
            if (pipefd[0] != -1) {
                close(pipefd[0]);
            }
            if (pipefd[1] != -1) {
                close(pipefd[1]);
            }

            if (applyRedirections(pipeline.commands[i]) != 0) {
                std::cout.flush();
                std::cerr.flush();
                _exit(1);
            }

            if (!pipeline.commands[i].args.empty() &&
                Builtins::isBuiltin(pipeline.commands[i].args[0])) {
                std::vector<std::string> args = expandWildcards(pipeline.commands[i].args);
                int code = Builtins::run(args, env, false);
                std::cout.flush();
                std::cerr.flush();
                _exit(code);
            }

            std::vector<std::string> args = expandWildcards(pipeline.commands[i].args);
            std::vector<char *> argv;
            for (auto &arg : args) {
                argv.push_back(const_cast<char *>(arg.c_str()));
            }
            argv.push_back(nullptr);

            execvp(argv[0], argv.data());
            perror("execvp");
            std::cout.flush();
            std::cerr.flush();
            _exit(127);
        }

        if (doTrace) {
            Trace::CommandTiming tcmd;
            tcmd.cmd = !pipeline.commands[i].args.empty() ? pipeline.commands[i].args[0] : "";
            tcmd.pid = pid;
            tcmd.startMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(commandStart - traceStartTime)
                    .count();
            tcmd.durationMs = 0;
            evt.commands.push_back(tcmd);
        }

        if (i + 1 == pipeline.commands.size()) {
            lastCommandPid = pid;
        }

        if (processGroup == -1) {
            processGroup = pid;
        }
        setpgid(pid, processGroup);
        if (prevRead != -1) {
            close(prevRead);
        }
        if (pipefd[1] != -1) {
            close(pipefd[1]);
        }
        prevRead = pipefd[0];
    }

    if (prevRead != -1) {
        close(prevRead);
    }

    if (background) {
        Jobs::add(processGroup, pipelineToString(pipeline));
        return 0;
    }

    const std::string commandText = pipelineToString(pipeline);

    const bool terminalHandoff = handoffTerminalTo(processGroup);

    while (true) {
        int status = 0;
        pid_t rc = waitpid(-processGroup, &status, WUNTRACED);
        if (rc > 0) {
            if (doTrace) {
                auto finishedAt = std::chrono::steady_clock::now();
                for (auto &command : evt.commands) {
                    if (command.pid == rc) {
                        auto commandStartTime =
                            traceStartTime + std::chrono::milliseconds(command.startMs);
                        command.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                 finishedAt - commandStartTime)
                                                 .count();
                        break;
                    }
                }
            }
            if (WIFSTOPPED(status)) {
                Jobs::add(processGroup, commandText, JobInfo::State::Stopped);
                lastStatus = 128 + WSTOPSIG(status);
                break;
            }
            if (rc == lastCommandPid) {
                if (WIFEXITED(status)) {
                    lastStatus = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    lastStatus = 128 + WTERMSIG(status);
                }
            }
            continue;
        }
        if (rc == -1 && errno == EINTR) {
            continue;
        }
        if (rc == -1 && errno == ECHILD) {
            break;
        }
        perror("waitpid");
        lastStatus = 1;
        break;
    }

    if (terminalHandoff) {
        pid_t shellPgid = Jobs::shellProcessGroup();
        if (tcsetpgrp(STDIN_FILENO, shellPgid > 0 ? shellPgid : getpgrp()) < 0) {
            perror("tcsetpgrp");
        }
    }

    if (doTrace) {
        auto traceEndTime = std::chrono::steady_clock::now();
        evt.processGroup = processGroup;
        evt.exitCode = lastStatus;
        evt.elapsedMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(traceEndTime - traceStartTime)
                .count();
        Trace::emit(evt, mode);
    }

    return lastStatus;
}
int evaluateNode(const Expr *node, Environment &env);

int evaluate(const Expr *node, Environment &env) {
    if (node == nullptr) {
        return 0;
    }
    if (!node->assignments.empty()) {
        ScopedAssignments scoped(env, node->assignments);
        return evaluateNode(node, env);
    }
    return evaluateNode(node, env);
}

int evaluateNode(const Expr *node, Environment &env) {
    if (node == nullptr) {
        return 0;
    }
    if (node->kind == ExprKind::Pipeline) {
        return executePipeline(node->pipeline, env, node->runInBackground);
    }
    if (node->kind == ExprKind::Subshell) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        }
        if (pid == 0) {
            int code = evaluate(node->left.get(), env);
            std::cout.flush();
            std::cerr.flush();
            _exit(code);
        }

        int status = 0;
        pid_t waited = -1;
        while ((waited = waitpid(pid, &status, 0)) < 0 && errno == EINTR) {
        }
        if (waited < 0) {
            perror("waitpid");
            return 1;
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);
        }
        return 1;
    }
    if (node->kind == ExprKind::Sequence) {
        evaluate(node->left.get(), env);
        return evaluate(node->right.get(), env);
    }
    if (node->kind == ExprKind::And) {
        int left = evaluate(node->left.get(), env);
        if (left != 0) {
            return left;
        }
        return evaluate(node->right.get(), env);
    }
    int left = evaluate(node->left.get(), env);
    if (left == 0) {
        return left;
    }
    return evaluate(node->right.get(), env);
}
} // namespace

int Executor::execute(const ExprPtr &root, Environment &env) { return evaluate(root.get(), env); }
