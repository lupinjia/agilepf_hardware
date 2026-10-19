#pragma once

#include "monitor_types.hpp"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>

namespace agilepf::monitor {

struct Options {
    ViewConfig view;
    std::string format = "text";
    bool once = false;
    double wait_timeout_ms = 5000;
    bool help = false;
};

inline bool parseDouble(const std::string& value, double& result) {
    if (value.empty()) return false;
    char* end = nullptr;
    errno = 0;
    const double parsed = std::strtod(value.c_str(), &end);
    if (errno == ERANGE || end != value.c_str() + value.size() || !std::isfinite(parsed)) return false;
    result = parsed;
    return true;
}

inline bool parseOptions(int argc, char** argv, Options& options, std::string& error) {
    constexpr int max_domain = 232; // DDSI-RTPS domain ID range.
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") { options.help = true; continue; }
        if (arg == "--once") { options.once = true; continue; }
        if (arg.empty() || arg.rfind("--", 0) != 0) { error = "unexpected operand: " + arg; return false; }
        if (i + 1 >= argc) { error = "missing value for " + arg; return false; }
        const std::string value(argv[++i]);
        if (arg == "--domain") {
            if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) { error = "invalid domain: " + value; return false; }
            errno = 0;
            char* end = nullptr;
            const long parsed = std::strtol(value.c_str(), &end, 10);
            if (errno || end != value.c_str() + value.size() || parsed < 0 || parsed > max_domain) { error = "domain must be between 0 and 232"; return false; }
            options.view.domain = static_cast<int>(parsed);
        } else if (arg == "--interface") {
            if (value.empty()) { error = "interface must not be empty"; return false; }
            options.view.interface_name = value;
        } else if (arg == "--topic") {
            if (value.empty()) { error = "topic must not be empty"; return false; }
            options.view.topic = value;
        } else if (arg == "--rate") {
            double parsed;
            if (!parseDouble(value, parsed) || parsed < 0.1 || parsed > 100) { error = "rate must be between 0.1 and 100 Hz"; return false; }
            options.view.refresh_hz = parsed;
        } else if (arg == "--timeout-ms") {
            double parsed;
            if (!parseDouble(value, parsed) || parsed <= 0 || parsed > 86400000) { error = "timeout-ms must be positive and at most 86400000"; return false; }
            options.view.stale_after_ms = parsed;
        } else if (arg == "--wait-timeout-ms") {
            double parsed;
            if (!parseDouble(value, parsed) || parsed <= 0 || parsed > 86400000) { error = "wait-timeout-ms must be positive and at most 86400000"; return false; }
            options.wait_timeout_ms = parsed;
        } else if (arg == "--format") {
            if (value != "text" && value != "json") { error = "format must be text or json"; return false; }
            options.format = value;
        } else { error = "unknown option: " + arg; return false; }
    }
    return true;
}

} // namespace agilepf::monitor
