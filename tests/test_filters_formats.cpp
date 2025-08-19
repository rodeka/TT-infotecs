#include "logging/logger.hpp"
#include <unistd.h>

using logging::Logger;
using logging::Severity;

static std::string unique_file(const char *tag)
{
    return std::string("log_") + tag + "_" + std::to_string(::getpid()) + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".txt";
}
