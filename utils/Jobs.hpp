#pragma once

#include <sys/types.h>
#include <cstdint>
#include <string>
#include <vector>

struct JobInfo {
    enum class State : std::uint8_t { Running, Stopped, Done };

    int id;
    pid_t pgid;
    std::string command;
    State state;
};

class Jobs {
  public:
    static void initShellProcessGroup(pid_t pgid);
    static int add(pid_t pgid, const std::string &command,
                   JobInfo::State state = JobInfo::State::Running);
    static void reapFinished();
    static const std::vector<JobInfo> &list();
    static int fg(int jobId);
    static int bg(int jobId);
    static pid_t shellProcessGroup();

  private:
    static int nextId_;
    static pid_t shellPgid_;
    static std::vector<JobInfo> jobs_;
};
