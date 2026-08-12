#include "../utils/Jobs.hpp"

#include <iostream>
#include <string>
#include <vector>

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
