#include "../utils/Jobs.hpp"

#include <iostream>

int builtin_jobs() {
    const auto &all = Jobs::list();
    for (const auto &job : all) {
        const char *state = "Done";
        if (job.state == JobInfo::State::Running) {
            state = "Running";
        } else if (job.state == JobInfo::State::Stopped) {
            state = "Stopped";
        }
        std::cout << "[" << job.id << "] " << state << " " << job.command << "\n";
    }
    return 0;
}
