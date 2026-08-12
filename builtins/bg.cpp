#include "../utils/Jobs.hpp"

#include <iostream>
#include <string>
#include <vector>

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
