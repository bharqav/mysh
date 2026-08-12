#include "Trace.hpp"

#include <algorithm>
#include <iostream>
#include <string>

namespace {

std::string timelineBar(long long durationMs) {
    long long width = std::max<long long>(1, std::min<long long>(durationMs, 24));
    return std::string(static_cast<std::size_t>(width), '-');
}

std::string jsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char c : value) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                out += "?";
            } else {
                out.push_back(c);
            }
            break;
        }
    }
    return out;
}

std::string dotEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

void emitSimple(const Trace::Event& evt) {
    std::cerr << "[TRACE] " << evt.description << " (pgid " << evt.processGroup << ") => "
              << evt.exitCode << " (" << evt.elapsedMs << "ms)\n";
}

void emitJson(const Trace::Event& evt) {
    std::cerr << "{\n";
    std::cerr << "  \"description\": \"" << jsonEscape(evt.description) << "\",\n";
    std::cerr << "  \"process_group\": " << evt.processGroup << ",\n";
    std::cerr << "  \"commands\": [\n";
    for (std::size_t i = 0; i < evt.commands.size(); ++i) {
        std::cerr << "    {\"cmd\": \"" << jsonEscape(evt.commands[i].cmd) << "\", \"pid\": "
                  << evt.commands[i].pid
                  << ", \"start_ms\": " << evt.commands[i].startMs
                  << ", \"duration_ms\": " << evt.commands[i].durationMs << "}";
        if (i + 1 < evt.commands.size()) {
            std::cerr << ",";
        }
        std::cerr << "\n";
    }
    std::cerr << "  ],\n";
    std::cerr << "  \"exit_code\": " << evt.exitCode << ",\n";
    std::cerr << "  \"elapsed_ms\": " << evt.elapsedMs << "\n";
    std::cerr << "}\n";
}

void emitGraph(const Trace::Event& evt) {
    std::cerr << "[TRACE GRAPH] " << evt.description << "\n";
    for (std::size_t i = 0; i < evt.commands.size(); ++i) {
        std::cerr << "  " << evt.commands[i].cmd << " (pid " << evt.commands[i].pid << ", "
                  << evt.commands[i].durationMs << "ms)\n";
        if (i + 1 < evt.commands.size()) {
            std::cerr << "     | (pipe)\n";
            std::cerr << "     v\n";
        }
    }
    std::cerr << "  Exit: " << evt.exitCode << ", Time: " << evt.elapsedMs << "ms\n";
}

void emitTimeline(const Trace::Event& evt) {
    std::cerr << "[TRACE TIMELINE] " << evt.description << "\n";
    for (const auto& command : evt.commands) {
        std::string label = command.cmd.empty() ? "<command>" : command.cmd;
        std::cerr << "  " << label;
        if (label.size() < 12) {
            std::cerr << std::string(12 - label.size(), ' ');
        }
        std::cerr << " " << timelineBar(command.durationMs) << " " << command.durationMs << "ms\n";
    }
    const std::string totalLabel = "total";
    std::cerr << "  total";
    if (totalLabel.size() < 12) {
        std::cerr << std::string(12 - totalLabel.size(), ' ');
    }
    std::cerr << " " << timelineBar(evt.elapsedMs) << " " << evt.elapsedMs << "ms\n";
}

void emitDot(const Trace::Event& evt) {
    std::cerr << "digraph mysh_trace {\n";
    std::cerr << "  rankdir=LR;\n";
    std::cerr << "  label=\"exit=" << evt.exitCode << ", total=" << evt.elapsedMs << "ms\";\n";
    std::cerr << "  labelloc=t;\n";
    for (std::size_t i = 0; i < evt.commands.size(); ++i) {
        const auto& command = evt.commands[i];
        std::string node = "n" + std::to_string(i);
        std::string label = (command.cmd.empty() ? std::string("<command>") : command.cmd) +
                            "\\npid=" + std::to_string(command.pid) +
                            "\\n" + std::to_string(command.durationMs) + "ms";
        std::cerr << "  " << node << " [shape=box, label=\"" << dotEscape(label) << "\"];\n";
        if (i + 1 < evt.commands.size()) {
            std::cerr << "  " << node << " -> n" << (i + 1) << " [label=\"pipe\"];\n";
        }
    }
    std::cerr << "}\n";
}

} // namespace

namespace Trace {

bool enabled(const Environment& env) {
    return !env.get("MYSH_TRACE").empty() && env.get("MYSH_TRACE") != "0";
}

std::string mode(const Environment& env) {
    std::string traceMode = env.get("MYSH_TRACE");
    if (traceMode.empty() || traceMode == "1") {
        return "simple";
    }
    return traceMode;
}

void emit(const Event& evt, const std::string& mode) {
    if (mode == "json") {
        emitJson(evt);
    } else if (mode == "graph") {
        emitGraph(evt);
    } else if (mode == "timeline") {
        emitTimeline(evt);
    } else if (mode == "dot") {
        emitDot(evt);
    } else {
        emitSimple(evt);
    }
}

} // namespace Trace
