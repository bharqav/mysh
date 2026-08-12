#include "Jobs.hpp"

#include <csignal>
#include <cerrno>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

int Jobs::nextId_ = 1;
pid_t Jobs::shellPgid_ = -1;
std::vector<JobInfo> Jobs::jobs_;

void Jobs::initShellProcessGroup(pid_t pgid) { shellPgid_ = pgid; }

pid_t Jobs::shellProcessGroup() { return shellPgid_; }

int Jobs::add(pid_t pgid, const std::string &command, JobInfo::State state) {
    for (auto &existing : jobs_) {
        if (existing.pgid == pgid) {
            existing.command = command;
            existing.state = state;
            return existing.id;
        }
    }
    JobInfo job{nextId_++, pgid, command, state};
    jobs_.push_back(job);
    std::cout << "[" << job.id << "] " << job.pgid << "\n";
    return job.id;
}

void Jobs::reapFinished() {
    for (auto &job : jobs_) {
        if (job.state == JobInfo::State::Done) {
            continue;
        }
        bool hasChildren = false;
        int status = 0;
        while (true) {
            pid_t rc = waitpid(-job.pgid, &status, WNOHANG);
            if (rc > 0) {
                hasChildren = true;
                continue;
            }
            if (rc == 0) {
                hasChildren = true;
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == ECHILD) {
                hasChildren = false;
            }
            break;
        }
        if (!hasChildren) {
            job.state = JobInfo::State::Done;
            std::cout << "[" << job.id << "] Done " << job.command << "\n";
        }
    }
}

const std::vector<JobInfo> &Jobs::list() { return jobs_; }

namespace {
bool assignTerminalTo(pid_t pgid) {
    if (!isatty(STDIN_FILENO) || pgid <= 0) {
        return false;
    }
    if (tcsetpgrp(STDIN_FILENO, pgid) < 0) {
        perror("tcsetpgrp");
        return false;
    }
    return true;
}
} // namespace

int Jobs::fg(int jobId) {
    for (auto &job : jobs_) {
        if (job.id != jobId) {
            continue;
        }
        if (job.state == JobInfo::State::Done) {
            std::cerr << "fg: job already finished\n";
            return 1;
        }
        job.state = JobInfo::State::Running;
        const bool terminalHandoff = assignTerminalTo(job.pgid);
        kill(-job.pgid, SIGCONT);
        int status = 0;
        int lastStatus = 0;
        while (true) {
            pid_t waitResult = waitpid(-job.pgid, &status, WUNTRACED);
            if (waitResult > 0) {
                if (WIFSTOPPED(status)) {
                    if (terminalHandoff) {
                        assignTerminalTo(shellPgid_ > 0 ? shellPgid_ : getpgrp());
                    }
                    job.state = JobInfo::State::Stopped;
                    std::cout << "[" << job.id << "] Stopped " << job.command << "\n";
                    constexpr int SIGNAL_BASE = 128;
                    return SIGNAL_BASE + WSTOPSIG(status);
                }
                if (WIFEXITED(status)) {
                    lastStatus = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    constexpr int SIGNAL_BASE = 128;
                    lastStatus = SIGNAL_BASE + WTERMSIG(status);
                }
                continue;
            }
            if (waitResult == -1 && errno == EINTR) {
                continue;
            }
            break;
        }
        if (terminalHandoff) {
            assignTerminalTo(shellPgid_ > 0 ? shellPgid_ : getpgrp());
        }
        job.state = JobInfo::State::Done;
        return lastStatus;
    }
    std::cerr << "fg: no such job\n";
    return 1;
}

int Jobs::bg(int jobId) {
    for (auto &job : jobs_) {
        if (job.id != jobId) {
            continue;
        }
        if (job.state == JobInfo::State::Done) {
            std::cerr << "bg: job already finished\n";
            return 1;
        }
        job.state = JobInfo::State::Running;
        kill(-job.pgid, SIGCONT);
        std::cout << "[" << job.id << "] " << job.command << " &\n";
        return 0;
    }
    std::cerr << "bg: no such job\n";
    return 1;
}
